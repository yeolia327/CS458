#include <cstdio>
#include <string>
#include <iostream>
#include <sstream>
#include <map>
#include <utility>
#include <fstream>
#include <vector>
#include <regex>

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Basic/TargetOptions.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Parse/ParseAST.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Rewrite/Frontend/Rewriters.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/IntrusiveRefCntPtr.h"
#include "clang/Lex/HeaderSearch.h"
#include "clang/Frontend/Utils.h"

using namespace clang;
using namespace std;

int total_branches = 0;
int id = 0;
StringRef funcName;
StringRef fileName;
bool funcNamePrinted = false;
bool funcUsedInMain = false;
bool enumCheck = false;
FunctionDecl* mainF = NULL;
vector<string> initList;

class MyASTVisitor : public RecursiveASTVisitor<MyASTVisitor>
{
public:
    MyASTVisitor(Rewriter &R, const LangOptions &langOptions)
        : TheRewriter(R), LangOpts(langOptions)
    {}
    bool VisitStmt(Stmt *s) {
        // Fill out this function for your homework
        if(enumCheck)
            return true;
        std::string str1;
        llvm::raw_string_ostream os(str1);
        SourceManager &m_sourceManager = TheRewriter.getSourceMgr();


        // s->printPretty(os, NULL, LangOpts);
        // llvm::outs() << os.str();
        // auto stmtName = s->getStmtClassName();
        // llvm::outs() << stmtName << "\n";

        bool check = false;

        if(isa<IfStmt>(s) || isa<ForStmt>(s) || isa<WhileStmt>(s) || isa<DoStmt>(s) || isa<DefaultStmt>(s) || isa<CaseStmt>(s) || isa<ConditionalOperator>(s))
            check = true;

        if(isa<SwitchStmt>(s)){

            bool defaultCheck = false;
            SwitchStmt *SS = dyn_cast<SwitchStmt>(s);
            for (SwitchCase *SC = SS->getSwitchCaseList(); SC; SC = SC->getNextSwitchCase()){
                if(isa<DefaultStmt>(SC)){
                    defaultCheck = true;
                    break;
                }
            }
            if(!defaultCheck){
                total_branches++;
                check = true;
            }
        } 

        SourceLocation startLocation = s->getBeginLoc();
        unsigned int lineNum = m_sourceManager.getExpansionLineNumber(startLocation);
        unsigned int columnNum = m_sourceManager.getExpansionColumnNumber(startLocation);

        if(funcUsedInMain || check)
            if(!funcNamePrinted){
                llvm::outs() << "function: " <<  funcName << '\n';
                funcNamePrinted = true;
                fileName = m_sourceManager.getFilename(startLocation);
            }


        if(check){
            // SourceLocation startLocation = s->getBeginLoc();
            // unsigned int lineNum = m_sourceManager.getExpansionLineNumber(startLocation);
            // unsigned int columnNum = m_sourceManager.getExpansionColumnNumber(startLocation);


            // if(!funcNamePrinted){
            //     llvm::outs() << "function: " <<  funcName << '\n';
            //     funcNamePrinted = true;
            //     fileName = m_sourceManager.getFilename(startLocation);
            // }

            if(isa<IfStmt>(s)){

                IfStmt *ifStmt = cast<IfStmt>(s);
                Expr * condition = ifStmt->getCond();

                std::string temp;
                llvm::raw_string_ostream condString(str1);
                condition->printPretty(condString, NULL, LangOpts);
                // llvm::outs() << condString.str() << "\n";

                string conditionString = regex_replace(condString.str(), regex("\""), "\'");


                tok::TokenKind tokenKind = clang::tok::r_paren;
                SourceLocation next_after_loc = clang::Lexer::findLocationAfterToken(condition->getEndLoc(), tokenKind, m_sourceManager, LangOpts, false);


                TheRewriter.InsertTextAfter(condition->getBeginLoc(), "((");

                string inputString = string(") ? (mycov_onTrueCondition (") + 
                    to_string(id) +string(")) : (mycov_onFalseCondition(") + 
                    to_string(id) + string(")))");

                TheRewriter.InsertTextAfter(ifStmt->getRParenLoc().getLocWithOffset(1), inputString);

                string init = string(" initCondition(") + to_string(id) + string(",\"") + conditionString + string("\",") + to_string(lineNum) + string(", 2") +  string(");\n");
                if(mainF){
                    TheRewriter.InsertTextBefore(mainF->getBody()->getBeginLoc().getLocWithOffset(1), init);
                }
                else
                    initList.push_back(init);


                llvm::outs() << "        If      ID: ";
                total_branches += 2;
            }
            else if(isa<ForStmt>(s)){
                ForStmt *forStmt = cast<ForStmt>(s);
                Expr * condition = forStmt->getCond();
                if(condition == NULL){
                    return true;
                } 
 
                std::string temp;
                llvm::raw_string_ostream condString(str1);
                condition->printPretty(condString, NULL, LangOpts);
                // llvm::outs() << condString.str() << "\n";

                string conditionString = regex_replace(condString.str(), regex("\""), "\'");

                TheRewriter.InsertTextAfter(condition->getBeginLoc(), "((");

                tok::TokenKind tokenKind = clang::tok::semi;
                // SourceLocation next_after_loc = clang::Lexer::findLocationAfterToken(condition->getEndLoc(), tokenKind, m_sourceManager, LangOpts, false);
                auto nextToken = clang::Lexer::findNextToken(condition->getEndLoc(), m_sourceManager, LangOpts);

                string inputString = string(") ? (mycov_onTrueCondition (") + 
                    to_string(id) + string(")) : (mycov_onFalseCondition(") + 
                    to_string(id) + string(")))\n");

                // TheRewriter.InsertTextBefore(next_after_loc.getLocWithOffset(-1), inputString);
                TheRewriter.InsertTextBefore(nextToken->getLocation(), inputString);

                string init = string(" initCondition(") + to_string(id) + string(",\"") + conditionString + string("\",") + to_string(lineNum) + string(", 2") +  string(");\n");
                if(mainF){
                    TheRewriter.InsertTextBefore(mainF->getBody()->getBeginLoc().getLocWithOffset(1), init);
                }
                else
                    initList.push_back(init);


                llvm::outs() << "        For      ID: ";
                total_branches += 2;
            }
            else if(isa<WhileStmt>(s)){
                WhileStmt *whileStmt = cast<WhileStmt>(s);
                Expr * condition = whileStmt->getCond();

                std::string temp;
                llvm::raw_string_ostream condString(str1);
                condition->printPretty(condString, NULL, LangOpts);

                string conditionString = regex_replace(condString.str(), regex("\""), "\'");


                TheRewriter.InsertTextAfter(condition->getBeginLoc(), "((");

                tok::TokenKind tokenKind = clang::tok::r_paren;
                SourceLocation next_after_loc = clang::Lexer::findLocationAfterToken(condition->getEndLoc(), tokenKind, m_sourceManager, LangOpts, false);

                string inputString = string(") ? (mycov_onTrueCondition (") + 
                    to_string(id) + string(")) : (mycov_onFalseCondition(") + 
                    to_string(id) + string(")))");

                // TheRewriter.InsertTextBefore(next_after_loc, inputString);
                TheRewriter.InsertTextAfter(whileStmt->getRParenLoc().getLocWithOffset(1), inputString);


                string init = string(" initCondition(") + to_string(id) + string(",\"") + conditionString + string("\",") + to_string(lineNum) + string(", 2") +  string(");\n");
                if(mainF){
                    TheRewriter.InsertTextBefore(mainF->getBody()->getBeginLoc().getLocWithOffset(1), init);
                }
                else
                    initList.push_back(init);


                llvm::outs() << "        While      ID: ";
                total_branches += 2;
            }
            else if(isa<DoStmt>(s)){
                DoStmt *doStmt = cast<DoStmt>(s);
                Expr * condition = doStmt->getCond();

                std::string temp;
                llvm::raw_string_ostream condString(str1);
                condition->printPretty(condString, NULL, LangOpts);

                string conditionString = regex_replace(condString.str(), regex("\""), "\'");

                TheRewriter.InsertTextAfter(condition->getBeginLoc(), "((");

                tok::TokenKind tokenKind = clang::tok::r_paren;
                SourceLocation next_after_loc = clang::Lexer::findLocationAfterToken(condition->getEndLoc(), tokenKind, m_sourceManager, LangOpts, false);

                string inputString = string(") ? (mycov_onTrueCondition (") + 
                    to_string(id) + string(")) : (mycov_onFalseCondition(") + 
                    to_string(id) + string(")))");

                TheRewriter.InsertTextBefore(next_after_loc, inputString);

                string init = string(" initCondition(") + to_string(id) + string(",\"") + conditionString + string("\",") + to_string(lineNum) + string(", 2") +  string(");\n");
                if(mainF){
                    TheRewriter.InsertTextBefore(mainF->getBody()->getBeginLoc().getLocWithOffset(1), init);
                }
                else
                    initList.push_back(init);



                llvm::outs() << "        Do      ID: ";
                total_branches += 2;
            }   
            else if(isa<DefaultStmt>(s)){
                DefaultStmt *defaultStmt = cast<DefaultStmt>(s);
                Stmt * condition = defaultStmt->getSubStmt();

                std::string temp;
                llvm::raw_string_ostream condString(str1);
                condition->printPretty(condString, NULL, LangOpts);


                tok::TokenKind tokenKind = clang::tok::colon;
                SourceLocation next_after_loc = clang::Lexer::findLocationAfterToken(s->getBeginLoc(), tokenKind, m_sourceManager, LangOpts, false);

                string inputString = string("mycov_onTrueCondition (") + 
                    to_string(id) + string(");");

                TheRewriter.InsertTextAfter(next_after_loc, inputString);

                string init = string(" initCondition(") + to_string(id) + string(",\"") + string("default") + string("\",") + to_string(lineNum) + string(", 1") + string(");\n");
                if(mainF){
                    TheRewriter.InsertTextBefore(mainF->getBody()->getBeginLoc().getLocWithOffset(1), init);
                }
                else
                    initList.push_back(init);

                llvm::outs() << "        Default      ID: ";
                total_branches++;
            } 
            else if(isa<CaseStmt>(s)){
                CaseStmt *caseStmt = cast<CaseStmt>(s);
                Stmt * condition = caseStmt->getLHS();

                std::string temp;
                llvm::raw_string_ostream condString(str1);
                condition->printPretty(condString, NULL, LangOpts);

                string conditionString = regex_replace(condString.str(), regex("\""), "\'");

                tok::TokenKind tokenKind = clang::tok::colon;
                SourceLocation next_after_loc = clang::Lexer::findLocationAfterToken(caseStmt->getLHS()->getBeginLoc(), tokenKind, m_sourceManager, LangOpts, false);

                string inputString = string("mycov_onTrueCondition (") + 
                    to_string(id) +  string(");");

                TheRewriter.InsertTextAfter(next_after_loc, inputString);

                string init = string(" initCondition(") + to_string(id) + string(",\"") + conditionString + string("\",") + to_string(lineNum) + string(", 1") + string(");\n");
                if(mainF){
                    TheRewriter.InsertTextBefore(mainF->getBody()->getBeginLoc().getLocWithOffset(1), init);
                }
                else
                    initList.push_back(init);

                llvm::outs() << "        Case      ID: ";
                total_branches++;
            }
            else if(isa<SwitchStmt>(s)){
                SwitchStmt *switchStmt = cast<SwitchStmt>(s);
                Stmt * condition = switchStmt->getCond();

                std::string temp;
                llvm::raw_string_ostream condString(str1);
                condition->printPretty(condString, NULL, LangOpts);

                // tok::TokenKind tokenKind = clang::tok::colon;
                // SourceLocation next_after_loc = clang::Lexer::findLocationAfterToken(s->getBeginLoc(), tokenKind, m_sourceManager, LangOpts, false);

                string inputString = string("mycov_onTrueCondition (") + 
                    to_string(id) + string(");");

                TheRewriter.InsertTextAfter(switchStmt->getEndLoc(), inputString);

                string init = string(" initCondition(") + to_string(id) + string(",\"") + string("default") + string("\",") + to_string(lineNum) + string(", 1") + string(");\n");
                if(mainF){
                    TheRewriter.InsertTextBefore(mainF->getBody()->getBeginLoc().getLocWithOffset(1), init);
                }
                else
                    initList.push_back(init);

                llvm::outs() << "        ImpDef.      ID: ";
            }
            else if(isa<ConditionalOperator>(s)){

                ConditionalOperator *conditionalOperator = cast<ConditionalOperator>(s);
                Expr * condition = conditionalOperator->getCond();


                llvm::raw_string_ostream condString(str1);
                condition->printPretty(condString, NULL, LangOpts);

                string conditionString = regex_replace(condString.str(), regex("\""), "\'");

                TheRewriter.InsertTextAfter(condition->getBeginLoc(), "((");

                tok::TokenKind tokenKind = clang::tok::question;
                // SourceLocation next_after_loc = clang::Lexer::findLocationAfterToken(condition->getEndLoc(), tokenKind, m_sourceManager, LangOpts, false);
                auto nextToken = clang::Lexer::findNextToken(condition->getEndLoc(), m_sourceManager, LangOpts);

                string inputString = string(") ? (mycov_onTrueCondition (") + 
                    to_string(id) + string(")) : (mycov_onFalseCondition(") + 
                    to_string(id) + string(")))\n");

                // TheRewriter.InsertTextBefore(next_after_loc.getLocWithOffset(-1), inputString);
                TheRewriter.InsertTextBefore(nextToken->getLocation(), inputString);
 

                string init = string(" initCondition(") + to_string(id) + string(",\"") + conditionString + string("\",") + to_string(lineNum) + string(", 2") + string(");\n");
                if(mainF){
                    // tok::TokenKind tokenKindMain = clang::tok::l_brace;
                    // SourceLocation next_after_Mainloc = clang::Lexer::findLocationAfterToken(mainF->getBody()->getBeginLoc(), tokenKindMain, m_sourceManager, LangOpts, false);
                    // TheRewriter.InsertTextAfter(next_after_Mainloc, init);
                    TheRewriter.InsertTextBefore(mainF->getBody()->getBeginLoc().getLocWithOffset(1), init);
                }
                else
                    initList.push_back(init);
                llvm::outs() << "        ?      ID: ";
                total_branches += 2;
            } 
            llvm::outs() << id << "   Line: ";
            
            
            llvm::outs() << lineNum << "                Col: ";
            llvm::outs() << columnNum << "          Filename: ";
            // llvm::outs() << m_sourceManager.getFilename(startLocation) << '\n';
            llvm::outs() << fileName << '\n';

            id++;

        }
        // SourceLocation startLocation = s->getBeginLoc();
        // unsigned int lineNum = m_sourceManager.getExpansionLineNumber(startLocation);
        // unsigned int columnNum = m_sourceManager.getExpansionColumnNumber(startLocation);

        // SourceLocation endLocation = s->getEndLoc();
        // unsigned int lineNumEnd = m_sourceManager.getExpansionLineNumber(endLocation);
        // unsigned int columnNumEnd = m_sourceManager.getExpansionColumnNumber(endLocation);

        // llvm::outs() << lineNum << ", " << columnNum << "\n";
        // llvm::outs() << lineNumEnd << ", " << columnNumEnd << "\n";

        // s->printPretty(os, NULL, LangOpts);
        // llvm::outs() << os.str() << '\n';
        // llvm::outs() << "TYPE:" << s->getStmtClassName() << "\n";




        return true;
    }
    
    bool VisitFunctionDecl(FunctionDecl *f) {
        // Fill out this function for your homework
        SourceManager &m_sourceManager = TheRewriter.getSourceMgr();
        if(m_sourceManager.isInMainFile(f->getBeginLoc()))
            funcUsedInMain = true;
        else
            funcUsedInMain = false;

        string initializeCode = string("/*My code here*/\n") +
        string("int branchTF[10000][3] = {0};\n") + 
        string("int branchLine[10000] = {0};\n") +
        string("char conditionalExpression[10000][500] = {0};\n") +
        string("int mycov_onTrueCondition(int id){\n") +
        string(" branchTF[id][0]++;\n") +
        string(" printAll();\n") +
        string(" return 1;\n") +
        string("}\n") +
        string("int mycov_onFalseCondition(int id){\n") +
        string(" branchTF[id][1]++;\n") +
        string(" printAll();\n") +
        string(" return 0;\n") +
        string("}\n") +
        string("void printAll(){\n") +
        string(" remove(\"coverage.dat\");\n") +
        string(" FILE *fp = fopen(\"coverage.dat\", \"w\");\n") +
        string(" fprintf(fp, \"Line# | # then | # else | conditional expression\\n\");\n") +
        string(" int tot = 0;\n") +
        string(" int covered = 0;\n") +
        string(" int i=0;\n") +
        string(" while(i < 10000){\n") +
        string("  if(conditionalExpression[i][0] == '\\0')\n") +
        string("     break;\n") +
        string("  fprintf(fp, \"%d       %d       %d           \", branchLine[i], branchTF[i][0], branchTF[i][1]);\n") +
        string("  fprintf(fp, \"%s\\n\", conditionalExpression[i]);\n") +
        string("  if(branchTF[i][0])\n") +
        string("   covered++;\n") +
        string("  if(branchTF[i][1])\n") +
        string("   covered++;\n") +
        string("  tot += branchTF[i][2];\n") +
        string("  i++;\n") +
        string(" }\n") +
        string(" fprintf(fp, \"Covered: %d / Total: %d = %f%%\", covered, tot, covered*100.0/tot);\n") +
        string("}\n") +
        string("void initCondition(int id, const char * str, int lineNum, int branchNum){\n") +
        string(" for(int i=0; i < 500; i++){\n") +
        string("  if(str[i] == '\\0')\n") +
        string("   break;\n") +
        string("  conditionalExpression[id][i] = str[i];\n") +
        string(" }\n") +
        string(" branchLine[id] = lineNum;\n") +
        string(" branchTF[id][2] = branchNum;\n") +
        string("}\n");

        if(f->isMain()){
            mainF = f;
            TheRewriter.InsertTextBefore(f->getBeginLoc(), 
                initializeCode);
            if(!initList.empty()){
                for(int i=0; i < initList.size(); i++)
                    TheRewriter.InsertTextBefore(mainF->getBody()->getBeginLoc().getLocWithOffset(1), initList[i]);
            }
            // if(f->isNoReturn())
            //     llvm::outs() << "hello\n";
            // else
            //     llvm::outs() << "yes return\n";

        }


        // llvm::outs() << "function: " <<  f->getName() << '\n';

        funcName = f->getName();
        funcNamePrinted = false;
        
        
        return true;
    }

private:
    Rewriter &TheRewriter;
    const LangOptions &LangOpts;
};

class MyASTConsumer : public ASTConsumer
{
public:
    MyASTConsumer(Rewriter &R, const LangOptions &langOptions)
        : Visitor(R, langOptions) //initialize MyASTVisitor
    {}

    virtual bool HandleTopLevelDecl(DeclGroupRef DR) {
        for (DeclGroupRef::iterator b = DR.begin(), e = DR.end(); b != e; ++b) {
            // Travel each function declaration using MyASTVisitor
            if(isa<EnumDecl>(**b))
                enumCheck = true;
            else
                enumCheck = false;
                
            // llvm::outs() << (**b).getDeclKindName() << " " << isa<EnumDecl>(**b) << '\n';
            Visitor.TraverseDecl(*b);
        }
        return true;
    }

private:
    MyASTVisitor Visitor;
};


int main(int argc, char *argv[])
{
    if (argc != 2) {
        llvm::errs() << "Usage: kcov-branch-identify <filename>\n";
        return 1;
    }

    // CompilerInstance will hold the instance of the Clang compiler for us,
    // managing the various objects needed to run the compiler.
    CompilerInstance TheCompInst;
    
    // Diagnostics manage problems and issues in compile 
    TheCompInst.createDiagnostics(NULL, false);

    // Set target platform options 
    // Initialize target info with the default triple for our platform.
    auto TO = std::make_shared<TargetOptions>();
    TO->Triple = llvm::sys::getDefaultTargetTriple();
    TargetInfo *TI = TargetInfo::CreateTargetInfo(TheCompInst.getDiagnostics(), TO);
    TheCompInst.setTarget(TI);

    // FileManager supports for file system lookup, file system caching, and directory search management.
    TheCompInst.createFileManager();
    FileManager &FileMgr = TheCompInst.getFileManager();
    
    // SourceManager handles loading and caching of source files into memory.
    TheCompInst.createSourceManager(FileMgr);
    SourceManager &SourceMgr = TheCompInst.getSourceManager();
    
    // Prreprocessor runs within a single source file
    TheCompInst.createPreprocessor(TU_Module);

    // Add HeaderSearch Path
    Preprocessor &PP = TheCompInst.getPreprocessor();
    const llvm::Triple &HeaderSearchTriple = PP.getTargetInfo().getTriple();
    
    HeaderSearchOptions &hso = TheCompInst.getHeaderSearchOpts();

    // <Warning!!> -- Platform Specific Code lives here
    // This depends on A) that you're running linux and
    // B) that you have the same GCC LIBs installed that I do.
    /*
    $ gcc -xc -E -v -
    ..
      /usr/lib/gcc/x86_64-linux-gnu/4.9/include
      /usr/local/include
      /usr/lib/gcc/x86_64-linux-gnu/4.9/include-fixed
      /usr/include/x86_64-linux-gnu
      /usr/include
    End of search list.
    */
    const char *include_paths[] = {"/usr/local/include",
        "/usr/include/x86_64-linux-gnu",
        "/usr/lib/gcc/x86_64-linux-gnu/7/include",
        "/usr/include"};
    
    for (int i = 0; i < (sizeof(include_paths) / sizeof(include_paths[0])); i++) {
        hso.AddPath(include_paths[i], clang::frontend::Angled, false, false);
    }
    // </Warning!!> -- End of Platform Specific Code

    ApplyHeaderSearchOptions(PP.getHeaderSearchInfo(), hso,
                             PP.getLangOpts(), HeaderSearchTriple);
    
    // ASTContext holds long-lived AST nodes (such as types and decls) .
    TheCompInst.createASTContext();

    // A Rewriter helps us manage the code rewriting task.
    Rewriter TheRewriter;
    TheRewriter.setSourceMgr(SourceMgr, TheCompInst.getLangOpts());

    // Set the main file handled by the source manager to the input file.
    auto expect_filein = FileMgr.getFile(argv[1]);
    if (error_code ec = expect_filein.getError()) {
        llvm::errs() << "Error: " << ec.message() << '\n';
        return 1;
    } 
    const FileEntry *FileIn = expect_filein.get();
    SourceMgr.setMainFileID(SourceMgr.createFileID(FileIn, SourceLocation(), SrcMgr::C_User));
    
    // Inform Diagnostics that processing of a source file is beginning. 
    TheCompInst.getDiagnosticClient().BeginSourceFile(TheCompInst.getLangOpts(),&TheCompInst.getPreprocessor());
    
    // Create an AST consumer instance which is going to get called by ParseAST.
    MyASTConsumer TheConsumer(TheRewriter, TheCompInst.getLangOpts());

    // Parse the file to AST, registering our consumer as the AST consumer.
    ParseAST(TheCompInst.getPreprocessor(), &TheConsumer, TheCompInst.getASTContext());

    llvm::outs() << "Total number of branches: " << total_branches << "\n";

    const RewriteBuffer *RewriteBuf = TheRewriter.getRewriteBufferFor(SourceMgr.getMainFileID());
    string fileName = argv[1];
    fileName = fileName.substr(0, fileName.size()-2);
    ofstream output(fileName + "-cov.i");
    output << string(RewriteBuf->begin(), RewriteBuf->end());
    output.close();


    return 0;
}
