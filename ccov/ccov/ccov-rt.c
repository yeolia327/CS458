#include <stdio.h>
#include <stdlib.h>
#include <string.h>

FILE * fp ;

int total_cov_num=0;



//Branch Br for line, Br_cases for index
struct Br_cases{
	int index;    // 0 to case_num
	int col;
	int true;
	int false;
};

struct Br{
	int pos;    // line
	// int true;   // true count
	// int false;  // false count
	int case_num;
	struct Br_cases* list;
};

struct cases{ 
	int def;   // 0 for case, 1 for default
	int index; // case index (0 .. Sw->case_num-1)
	int value; // case value
	int count; // vase count
};

struct Sw{
	int pos;
	int case_num;       //total case number last case is always default
	int condition;
	struct cases* list;
};

struct cov {
	int indicator; // 0 for Br, 1 for Sw
	int pos;       // line number
	struct Br* br;
	struct Sw* sw;
};


struct cov* cov_list;  //list that contains every branch information

int compareCov(const void* a, const void* b) {
    const struct cov* s1 = (const struct cov*) a;
    const struct cov* s2 = (const struct cov*) b;
    return s1->pos > s2->pos;
}


// void printCov(){
// 	for(int i=0; i < total_cov_num; i++){
// 		printf("index: %d, indicator: %d, pos: %d\n", i, cov_list[i].indicator, cov_list[i].pos);
// 		if(!cov_list[i].indicator)
// 			printf("Br pos:%d, true: %d, false: %d\n", cov_list[i].br->pos, cov_list[i].br->true, cov_list[i].br->false);
// 		else{
// 			printf("Sw pos:%d, case_num: %d, condition: %d\n", cov_list[i].sw->pos, cov_list[i].sw->case_num, cov_list[i].sw->condition);
	
// 			for(int j=0; j < cov_list[i].sw->case_num; j++){
// 				printf("Sw cases default: %d, index: %d, value: %d, count: %d\n", cov_list[i].sw->list[j].def,
// 				cov_list[i].sw->list[j].index,
// 				cov_list[i].sw->list[j].value,
// 				cov_list[i].sw->list[j].count);
// 			}
// 		}
// 	}
// }
// 7.0 -> 0, 1
// 38.0 -> 0, 0
// 38.1 -> 0, 0
// 38.2 -> 1, 0

void prettyPrint(){
	int total_branch_num = 0;
	int covered_branch = 0;

	for(int i=0; i < total_cov_num; i++){
		if(!cov_list[i].indicator){
			struct Br * branch = cov_list[i].br;
			for(int j=0; j < branch->case_num; j++){
				struct Br_cases branch_case = branch->list[j];
				fprintf(fp, "br: %d.%d -> %d, %d\n", branch->pos, branch_case.index, branch_case.true, branch_case.false);
				total_branch_num += 2;
				covered_branch += (branch_case.true ? 1 : 0);
				covered_branch += (branch_case.false ? 1 : 0);
			}
		}
		else{
			total_branch_num += cov_list[i].sw->case_num;
			for(int j=0; j < cov_list[i].sw->case_num; j++){
				fprintf(fp, "sw: %d.%d -> %d, 0\n", cov_list[i].sw->pos, cov_list[i].sw->list[j].index, cov_list[i].sw->list[j].count);
				covered_branch += (cov_list[i].sw->list[j].count ? 1 : 0);
			}
		}
	}
	fprintf(fp, "Total: %d branches, Covered: %d branches", total_branch_num, covered_branch);


}

extern void _final_() {
	qsort(cov_list, total_cov_num, sizeof(struct cov), compareCov);
	prettyPrint();
	free(cov_list);
	fclose(fp) ;
}

extern void _init_coverage_data_(){
	cov_list = malloc(10000 * sizeof(struct cov));
	// if coverage.dat already exists, load data and build cov_list
	if((fp = fopen("coverage.dat", "r")) != NULL){
		// printf("file already exists, loading values\n");

		char strTemp[255];
		char * ptr;
		int ind, line, index, t, f;

		while( !feof( fp ) )
		{
			fgets( strTemp, sizeof(strTemp), fp );
			// printf( "%s", strTemp );
			if(strTemp[0] == 'T'){
				break;
			}else{
				ptr = strtok(strTemp, " ");
				ind = (ptr[0] == 'b' ? 0 : 1);
				ptr = strtok(NULL, ".");
				line = atoi(ptr);
				ptr = strtok(NULL, " ");
				index = atoi(ptr);
				ptr = strtok(NULL, " ");
				ptr = strtok(NULL, ",");
				t = atoi(ptr);
				ptr = strtok(NULL, "");
				f = atoi(ptr);
				// printf("ind: %d, line: %d, index: %d, t: %d, f: %d\n", ind, line, index, t, f);
			}

			if(!ind){
				if(index == 0){
					struct Br * target = malloc(sizeof(struct Br));
					target->pos = line;
					target->case_num = 1;
					target->list = malloc(1 * sizeof(struct Br_cases));

					target->list[0].index = 0;
					target->list[0].col = -1;
					target->list[0].true = t;
					target->list[0].false = f;

					cov_list[total_cov_num].indicator = 0;
					cov_list[total_cov_num].pos = line;
					cov_list[total_cov_num].br = target;
					cov_list[total_cov_num].sw = NULL;
					total_cov_num++;
				}
				else{
					struct Br *target = NULL;
					for(int i=0; i < total_cov_num; i++){
						if(cov_list[i].indicator == 0 && cov_list[i].pos == line){
							target = cov_list[i].br;
							break;
						}
					}

					target->case_num++;
					target->list = realloc(target->list, target->case_num * sizeof(struct cases));
					target->list[target->case_num-1].index = target->case_num-1;
					target->list[target->case_num-1].col = -1;
					target->list[target->case_num-1].true = t;
					target->list[target->case_num-1].false = f;
				}


			}else{
				if(index == 0){
					struct Sw * target = malloc(sizeof(struct Sw));
					target->pos = line;
					target->case_num = 1;
					target->condition = -1;
					target->list = malloc(1 * sizeof(struct cases));
					target->list[0].def = 1;    //fist of all, set def as default
					target->list[0].index = 0;
					target->list[0].value = -1;
					target->list[0].count = t;

					cov_list[total_cov_num].indicator = 1;
					cov_list[total_cov_num].pos = line;
					cov_list[total_cov_num].br = NULL;
					cov_list[total_cov_num].sw = target;
					total_cov_num++;
				}else{
					struct Sw *target = NULL;
					for(int i=0; i < total_cov_num; i++){
						if(cov_list[i].indicator == 1 && cov_list[i].pos == line){
							target = cov_list[i].sw;
							break;
						}
					}

					target->case_num++;
					target->list = realloc(target->list, target->case_num * sizeof(struct cases));
					target->list[target->case_num-2].def = 0;
					target->list[target->case_num-1].def = 1;  //if there exists additional case, set previous case as ordianry case
					target->list[target->case_num-1].index = index;
					target->list[target->case_num-1].value = -1;
					target->list[target->case_num-1].count = t;
				}

			}
		}
		fclose(fp);
		// printf("load complete\n");
		// fp = fopen("temp.dat", "w");
		// prettyPrint();
		// fclose(fp);

		// printf("total cov num: %d", total_cov_num);

	}


	fp = fopen("coverage.dat", "w") ;
	atexit(_final_) ;
}


// if coverage.data does not exists, this function will create new struct cov data
// if coverage.data exists, this function will just replace col value in struct Br_cases
// why? not initialize col data in _init_coverage_data_ ??
// I can't as coverage.data just contain line and index, no column value ex) 311.0 -> 0, 0 we can not extract col value here
// we need to manually append col value in Br when _br_initialize_ called.
// what about ordering in col? ordering of col is fixed.
extern void _br_initialize_(int line, int col){
	struct Br * target = NULL;

	for(int i=0; i < total_cov_num; i++){
		if(cov_list[i].indicator == 0 && cov_list[i].pos == line){
			target = cov_list[i].br;
			break;
		}
	}

	if(!target){
		// it means that original coverage.dat does not exists, we need to make a new sturct Br
		struct Br * target = malloc(sizeof(struct Br));
		target->pos = line;
		target->case_num = 1;
		target->list = malloc(1 * sizeof(struct Br_cases));

		target->list[0].index = 0;
		target->list[0].col = col;
		target->list[0].true = 0;
		target->list[0].false = 0;

		cov_list[total_cov_num].indicator = 0;
		cov_list[total_cov_num].pos = line;
		cov_list[total_cov_num].br = target;
		cov_list[total_cov_num].sw = NULL;
		total_cov_num++;
	}else{
		// two cases 1. no original coverage.dat, then there is no additional -1 col value, which means that we need to make new Br_cases
		// 2. there exists additional -1 col value, which means that there already exists original coverage.dat file and we need to add col value


		//case 1
		for(int i=0; i < target->case_num; i++){
			if(target->list[i].col == -1){
				target->list[i].col = col;
				return ;
			}
		}


		//case 2
		target->case_num++;
		target->list = realloc(target->list, target->case_num * sizeof(struct cases));
		target->list[target->case_num-1].index = target->case_num-1;
		target->list[target->case_num-1].col = col;
		target->list[target->case_num-1].true = 0;
		target->list[target->case_num-1].false = 0;
	}
}

extern void _sw_initialize_(int line, int num){
	// fprintf(fp, "sw initialize line: %d, num: %d\n", line, num);
	struct Sw * target = NULL;
	
	for(int i=0; i < total_cov_num; i++){
		if(cov_list[i].indicator == 1 && cov_list[i].pos == line){
			target = cov_list[i].sw;
			break;
		}
	}

	if(!target){
		target = malloc(sizeof(struct Sw));
		target->pos = line;
		target->case_num = num;
		target->list = malloc(num * sizeof(struct cases));
		for(int i=0; i < num; i++){
			target->list[i].def = (i == num-1 ? 1 : 0);
			target->list[i].index = i;
			target->list[i].count = 0;
		}

		cov_list[total_cov_num].indicator = 1;
		cov_list[total_cov_num].pos = line;
		cov_list[total_cov_num].br = NULL;
		cov_list[total_cov_num].sw = target;
		total_cov_num++;
	}
}

extern void _br_probe_(int line, int col, int value){
    // printf("br position: %d, col: %d, truth value: %d\n", line, col, value);
	struct Br * target = NULL;

	for(int i=0; i < total_cov_num; i++){
		if(cov_list[i].indicator == 0 && cov_list[i].pos == line){
			target = cov_list[i].br;
			break;
		}
	}

	for(int i=0; i < target->case_num; i++){
		if(target->list[i].col == col){
			if(value)
				target->list[i].true++;
			else
				target->list[i].false++;
			break;
		}
	}
}

// here is the step, First, _sw_probe_case_ called for every cases, which saves case values into struct case
// Then _sw_probe_checkCondition_ is called, which contains switch case condition value. Iterate sw case list, increment proper case count

extern void _sw_probe_checkCondition_(int line, int value){
    // fprintf(fp, "sw position: %d, condition: %d\n", line, value);

	struct Sw * target = NULL;
	
	for(int i=0; i < total_cov_num; i++){
		if(cov_list[i].indicator == 1 && cov_list[i].pos == line){
			target = cov_list[i].sw;
			break;
		}
	}

	if(!target){
		printf("switch case error!, case not matching switch");
		exit(-1);
	}

	for(int i=0; i < target->case_num; i++){
		if(i == target->case_num-1){
			target->list[i].count++;
			break;
		}else{
			if(value == target->list[i].value){
				target->list[i].count++;
				break;
			}
		}
	}
}

extern void _sw_probe_case_(int line, int num, int value){
    // fprintf(fp, "sw position: %d, case numbers: %d, case value: %d\n", line, num, value);
	struct Sw * target = NULL;

	for(int i=0; i < total_cov_num; i++){
		if(cov_list[i].indicator == 1 && cov_list[i].pos == line){
			target = cov_list[i].sw;
			break;
		}
	}

	if(!target){
		printf("switch case error!, case not matching switch");
		exit(-1);
	}

	target->list[num].value = value;
}
