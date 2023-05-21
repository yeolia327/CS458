#define DEBUG_TYPE "CCov"

#include <iostream>
#include <vector>
#include <utility>

#include "llvm/IR/DebugInfo.h"
#include "llvm/Pass.h"
#include "llvm/IR/PassManager.h"

#include "llvm/ADT/Statistic.h"

#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LegacyPassManager.h"

#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

/* CCov.cpp
 *
 * Currently, it is a 'blank' FunctionPass that gives no effect on the target 
 * program. You can implement your own FunctionPass by writing down your own 
 * code where the comment "Fill out" is.  
 * */

using namespace llvm;
using namespace std;

namespace {
  class CCov: public FunctionPass {
	public:
	
    	static char ID; // Pass identification, replacement for typeid

		// Fill out.

		CCov() : FunctionPass(ID) {}

		Type *intTy, *ptrTy, *voidTy, *boolTy; // These variables are to store the type instances for primitive types.
		
		FunctionCallee p_br_probe;
		FunctionCallee p_sw_probe_checkCondition;
		FunctionCallee p_sw_probe_case;
		FunctionCallee p_init_coverage_data;
		FunctionCallee p_br_initialize;
		FunctionCallee p_sw_initialize;


		vector<pair<int,int>> brIndexList;
		vector<pair<int,int>> swIndexList;


		IRBuilder<> * IRB;

		virtual bool doInitialization(Module &M) {
			/* doInitialization() is executed once per target module,
			 * and executed before any invocation of runOnFunction().
			 * This function is for initialization and the module level 
			 * instrumentation (e.g., add functions). */


			/* check if there is a function in a target program that conflicts
			 * with the probe functions */			
			if (M.getFunction(StringRef("_br_probe_")) != NULL) {
				errs() << "Error: function _br_probe_() already exists.\n" ;
				exit(1) ;
			}
			if (M.getFunction(StringRef("_sw_probe_checkCondition_")) != NULL) {
				errs() << "Error: function _sw_probe_checkCondition_() already exists.\n" ;
				exit(1) ;
			}
			if (M.getFunction(StringRef("_sw_probe_case_")) != NULL) {
				errs() << "Error: function _sw_probe_case_() already exists.\n" ;
				exit(1) ;
			}
			if (M.getFunction(StringRef("_init_coverage_data_")) != NULL) {
				errs() << "Error: function _init_coverage_data_() already exists.\n" ;
				exit(1) ;
			}
			if (M.getFunction(StringRef("_br_initialize_")) != NULL) {
				errs() << "Error: function _br_initialize_() already exists.\n" ;
				exit(1) ;
			}
			// errs() << "Running intwrite pass\n";

			/* store the type instances for primitive types */
			intTy = Type::getInt32Ty(M.getContext()) ;
			ptrTy = Type::getInt8PtrTy(M.getContext()) ;
			voidTy = Type::getVoidTy(M.getContext()) ;
			boolTy = Type::getInt1Ty(M.getContext()) ;

			Type * args_types_br[3] ;
			args_types_br[0] = intTy ; //Type::getInt32Ty(*ctx) ;	
			args_types_br[1] = intTy ; //Type::getInt32Ty(*ctx) ;
			args_types_br[2] = intTy ; //Type::getInt32Ty(*ctx) ;	
			p_br_probe = M.getOrInsertFunction("_br_probe_", FunctionType::get(voidTy, ArrayRef<Type *>(args_types_br), false));

			Type * args_types_sw_checkCondition[2] ;
			args_types_sw_checkCondition[0] = intTy ; //Type::getInt32Ty(*ctx) ;	
			args_types_sw_checkCondition[1] = intTy ; //Type::getInt32Ty(*ctx) ;	
			p_sw_probe_checkCondition = M.getOrInsertFunction("_sw_probe_checkCondition_", FunctionType::get(voidTy, ArrayRef<Type *>(args_types_sw_checkCondition), false));

			Type * args_types_sw_case[3] ;
			args_types_sw_case[0] = intTy ; //Type::getInt32Ty(*ctx) ;	
			args_types_sw_case[1] = intTy ; //Type::getInt32Ty(*ctx) ;	
			args_types_sw_case[2] = intTy ; //Type::getInt32Ty(*ctx) ;	
			p_sw_probe_case = M.getOrInsertFunction("_sw_probe_case_", FunctionType::get(voidTy, ArrayRef<Type *>(args_types_sw_case), false));


			p_init_coverage_data = M.getOrInsertFunction("_init_coverage_data_", FunctionType::get(voidTy, false)) ;


			Type * args_types_br_initialize[2] ;
			args_types_br_initialize[0] = intTy ;
			args_types_br_initialize[1] = intTy ;
			p_br_initialize = M.getOrInsertFunction("_br_initialize_", FunctionType::get(voidTy, ArrayRef<Type *>(args_types_br_initialize), false)) ;

			Type * args_types_sw_initialize[2] ;
			args_types_sw_initialize[0] = intTy ;
			args_types_sw_initialize[1] = intTy ;
			p_sw_initialize = M.getOrInsertFunction("_sw_initialize_", FunctionType::get(voidTy, ArrayRef<Type *>(args_types_sw_initialize), false)) ;


			/* add a function call to _init_ at the beginning of 
			 * the main function*/

			IRB = new IRBuilder<>(M.getContext());
			// Fill out.

			return true ;
		} // doInitialization.

		virtual bool doFinalization(Module &M) {
			/* This function is executed once per target module after
			 * all executions of runOnFunction() under the module. */

			// Fill out.
			// errs() << "finalize\n";

			Function * mainFunc = M.getFunction(StringRef("main"));
			if (mainFunc != NULL) {
				IRB->SetInsertPoint(mainFunc->getEntryBlock().getFirstNonPHIOrDbgOrLifetime());
				IRB->CreateCall(p_init_coverage_data, {}) ;



				for(int i=0; i < brIndexList.size(); i++){
					Value * args[2] ;
					args[0] = ConstantInt::get(intTy, brIndexList[i].first, false) ; // line
					args[1] = ConstantInt::get(intTy, brIndexList[i].second, false) ; // line
					IRB->CreateCall(p_br_initialize, args, Twine("")) ;
				}

				for(int i=0; i < swIndexList.size(); i++){
					Value * args_sw[2] ;
					args_sw[0] = ConstantInt::get(intTy, swIndexList[i].first, false) ;  // line 
					args_sw[1] = ConstantInt::get(intTy, swIndexList[i].second, false) ; // case nums
					IRB->CreateCall(p_sw_initialize, args_sw, Twine("")) ;
				}


			}

			return true ;
		} //doFinalization.

		virtual bool runOnFunction(Function &F) {
				/* This function is invoked once for every function in the target 
				* module by LLVM */

				// Fill out.
				for (Function::iterator itr = F.begin() ; itr != F.end() ; itr++) {
					runOnBasicBlock(*itr) ;
				}

		return true;
		} //runOnFunction.

		bool runOnBasicBlock (BasicBlock &B) {
			/* This function is invoked by runOnFunction() for each basic block
			 * in the function. Note that this is not invoked by LLVM and different
			 * from runOnBasicBlock() of BasicBlockPass.*/

			// Fill out.
			// errs() << "start\n";
			StringRef funcname = "unknown";
			DISubprogram * disubp = B.getParent()->getSubprogram();
			if (disubp) {
				funcname = disubp->getName();
			}


			for (BasicBlock::iterator i = B.begin() ; i != B.end() ; i++) {
				/* for each instruction of the basic block in order */

				if(i->getOpcode() == Instruction::Br){
					BranchInst * br = dyn_cast<BranchInst>(i);
					if(br->isUnconditional())
						continue;


					const DebugLoc &debugloc = br->getDebugLoc();

					int loc = -1;
					int col = -1;
					if (debugloc) {
						loc = debugloc.getLine();
						col = debugloc.getCol();
					}					
					Value * val = br->getOperand(0) ;  // br condition truth value
					
					// errs() << debugloc.getLine() << " " << debugloc.getCol() << '\n';
					//for initialize
					brIndexList.push_back({loc, col});

					IRB->SetInsertPoint(&(*i));
					
					Value * args[3] ;
					args[0] = ConstantInt::get(intTy, loc, false) ;
					args[1] = ConstantInt::get(intTy, col, false);
					args[2] = val ;
					IRB->CreateCall(p_br_probe, args, Twine("")) ;
					continue ;

				}

				if(i->getOpcode() == Instruction::Switch){
					SwitchInst * sw = dyn_cast<SwitchInst>(i);

					Value * val = sw->getCondition(); // the condition value

					IRB->SetInsertPoint(&(*i));


					const DebugLoc &debugloc = sw->getDebugLoc();

					int loc = -1;
					if (debugloc) {
						loc = debugloc.getLine();
					}

					//for initialize
					swIndexList.push_back({loc, sw->getNumCases()+1});

					for(SwitchInst::CaseIt it = sw->case_begin(); it != sw->case_end(); it++){
						Value * args[3] ;
						args[0] = ConstantInt::get(intTy, loc, false) ;
						args[1] = ConstantInt::get(intTy, it->getCaseIndex(), false) ;
						args[2] = it->getCaseValue() ; 
						IRB->CreateCall(p_sw_probe_case, args, Twine("")) ;

					}

					Value * args_checkCondition[2] ;
					args_checkCondition[0] = ConstantInt::get(intTy, loc, false) ;
					args_checkCondition[1] = val ; 
					IRB->CreateCall(p_sw_probe_checkCondition, args_checkCondition, Twine("")) ;	
					continue ;

				}
			}

			return true ;
		} // runOnBasicBlock.
  };
}

/* The code in the remaining part is to register this Pass to
 * LLVM Pass Manager such that LLVM/Clang can use it. */
char CCov::ID = 0;

static RegisterPass<CCov> X("CCov", "CCov Pass", false , false);

static void registerPass(const PassManagerBuilder &,
                         legacy::PassManagerBase &PM) {

  PM.add(new CCov());
}

static RegisterStandardPasses
    RegisterPassOpt(PassManagerBuilder::EP_ModuleOptimizerEarly, registerPass);

static RegisterStandardPasses
    RegisterPassO0(PassManagerBuilder::EP_EnabledOnOptLevel0, registerPass);