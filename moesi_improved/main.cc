/*******************************************************
                          main.cc
                  Ahmad Samih & Yan Solihin
                           2009
                {aasamih,solihin}@ece.ncsu.edu
********************************************************/

#include <stdlib.h>
#include <assert.h>
#include <fstream>
#include <string>
#include <iostream>
using namespace std;

#include "cache.h"

FILE *fp = NULL;
int p;
char op;
int address;

void custom(Cache **cacheArray, int num_processors, char *fname) {
    char *buf = (char *)malloc(13*sizeof(char));
    size_t len = 13;
    if((fp = fopen(fname, "r")) == NULL) {
        cout<< "Error opening trace file"<<endl;
        exit(-1);
    }
    
	while((getline(&buf, &len, fp))!= -1) {
        int ret_val = sscanf(buf, "%d %c %x", &p, &op, &address);
		if(ret_val == -1) {
			printf("sscanf failed\n");
			exit(0);
		}

		int cache_count = 0;
		int illinois = 0;
		int zero_count = 0;
		ret_val = 0;
		cacheLine *line = cacheArray[p]->findLine(address);
		if(op == 'r') {
			if(line == NULL || line->getFlags()==INVALID) {
				/* Coherence Miss */
				if(cacheArray[p]->checkTag(address) == 1) {
					cacheArray[p]->coherenceMiss();
				}

				cacheArray[p]->bus(BUSRD);
				for(int i=0;i<num_processors;i++) {
					if(i!=p) {
						ret_val = cacheArray[i]->custom_busRd(address);  //BusRd happens only when the cache is INVALID
						if(ret_val == 0) ++zero_count;
						else if(ret_val == 1) {
							cache_count++;
						} else if(ret_val == 2) {
							illinois = 1;
						}
					}
				}
				if(zero_count == num_processors-1) { 
					cacheArray[p]->mem();
					cacheArray[p]->memCmd();
					cacheArray[p]->memData();
					cacheArray[p]->bus(DATA);
				}

				for(int i=0; i<cache_count; i++) {
					cacheArray[p]->ctoc();
					cacheArray[p]->bus(DATA);
				}
				if(illinois == 1) {
					cacheArray[p]->ctoc();
					cacheArray[p]->bus(DATA);
				}
			}
		} else if(op == 'w') {
			cache_count = 0;
			illinois = 0;

			if(line == NULL || line->getFlags() == VALID) 
				cacheArray[p]->bus(BUSRDX);

			for(int i=0; i<num_processors;i++) {
				if(i!=p) {
					if(line == NULL || line->getFlags() == INVALID) {
						/* Coherence Misses */
						if(cacheArray[p]->checkTag(address) == 1) {
							cacheArray[p]->coherenceMiss();
						}

						ret_val = cacheArray[i]->custom_busRdX(address);
						if(ret_val == 0) ++zero_count;
						else if(ret_val == 1) { 
							cache_count++;
						} else if(ret_val == 2) {
							illinois = 1;
						}
					} else if(line->getFlags() == VALID) {
						cacheArray[i]->custom_busUpgr(address);
					}
				}
			}

			if(zero_count == num_processors-1) { 
				cacheArray[p]->mem();
				/*new code begins*/
				cacheArray[p]->memCmd();
				cacheArray[p]->memData();
				/*new code ends*/
				cacheArray[p]->bus(DATA);
			}
			for(int i=0; i<cache_count;i++) {
				cacheArray[p]->ctoc();
				cacheArray[p]->bus(DATA);
			}
			if(illinois == 1) {
				cacheArray[p]->ctoc();
				cacheArray[p]->bus(DATA);
			}
		}
		cacheArray[p]->Access_custom(address, op, p, cacheArray);
	}
}

void moesi(Cache **cacheArray, int num_processors, char *fname) {
    char *buf = (char *)malloc(13*sizeof(char));
    size_t len = 13;
    if((fp = fopen(fname, "r")) == NULL) {
        cout<< "Error opening trace file"<<endl;
        exit(-1);
    }

	while((getline(&buf, &len, fp))!= -1) {
        int ret_val = sscanf(buf, "%d %c %x", &p, &op, &address);
		if(ret_val == -1) {
			printf("sscanf failed\n");
			exit(0);
		}
		int cache_count = 0;
		int illinois = 0;
		int zero_count = 0;
		cacheLine *line = cacheArray[p]->findLine(address);
		if(op == 'r') {
			if(line == NULL || line->getFlags() == INVALID) {
				/* Coherence Miss */
				if(cacheArray[p]->checkTag(address) == 1) {
					cacheArray[p]->coherenceMiss();
				}

				cacheArray[p]->bus(BUSRD);
				for(int i=0; i<num_processors; i++) {
					if(i!=p) {
						ret_val = cacheArray[i]->moesi_busRd(address);
						if(ret_val == 0) ++zero_count;
						else if(ret_val == 1) {
							cache_count++;
						} else if(ret_val == 2) {
							illinois = 1;
						}
					}
				}
			}
			if(zero_count == num_processors-1) { 
				cacheArray[p]->mem();
				cacheArray[p]->memCmd();
				cacheArray[p]->memData();
				cacheArray[p]->bus(DATA);
			}
			for(int i=0; i<cache_count; i++) {
				cacheArray[p]->ctoc();
				cacheArray[p]->bus(DATA);
			}
			if(illinois == 1) { 
				cacheArray[p]->ctoc();
				cacheArray[p]->bus(DATA);
			}
		} else if(op == 'w') {
			cache_count = 0;
			illinois = 0;
			
			if(line == NULL || line->getFlags() == VALID || line->getFlags() == OWNER) 
				cacheArray[p]->bus(BUSRDX);
			
			for(int i=0; i<num_processors; i++) {
				if(i!=p) {
					if(line == NULL || line->getFlags() == INVALID) {
						/* Coherence Miss */
						if(cacheArray[p]->checkTag(address) == 1) {
							cacheArray[p]->coherenceMiss();
						}

						ret_val = cacheArray[i]->moesi_busRdX(address);
						if(ret_val == 0) ++zero_count;
						else if(ret_val == 1) { 
							cache_count++;
						} else if(ret_val == 2) {
							illinois = 1;
						}
					} else if(line->getFlags() == VALID || line->getFlags() == OWNER) {
						cacheArray[i]->moesi_busUpgr(address);
					}
				}
			}
			if(zero_count == num_processors-1) { 
				cacheArray[p]->mem();
				cacheArray[p]->memCmd();
				cacheArray[p]->memData();
				cacheArray[p]->bus(DATA);
			}
			for(int i=0; i<cache_count;i++) {
				cacheArray[p]->ctoc();
				cacheArray[p]->bus(DATA);
			}
			if(illinois == 1) {
				cacheArray[p]->ctoc();
				cacheArray[p]->bus(DATA);
			}
		}
		cacheArray[p]->Access_MOESI(address, op, p, cacheArray);
	}
}

void mesi(Cache **cacheArray, int num_processors, char *fname) {
    char *buf = (char *)malloc(13*sizeof(char));
    size_t len = 13;
    if((fp = fopen(fname, "r")) == NULL) {
        cout<< "Error opening trace file"<<endl;
        exit(-1);
    }
    
	while((getline(&buf, &len, fp))!= -1) {
        int ret_val = sscanf(buf, "%d %c %x", &p, &op, &address);
		if(ret_val == -1) {
			printf("sscanf failed\n");
			exit(0);
		}

		int cache_count = 0;
		int illinois = 0;
		int zero_count = 0;
		ret_val = 0;
		cacheLine *line = cacheArray[p]->findLine(address);
		if(op == 'r') {
			if(line == NULL || line->getFlags()==INVALID) {
				/* Coherence Miss */
				if(cacheArray[p]->checkTag(address) == 1) {
					cacheArray[p]->coherenceMiss();
				}

				cacheArray[p]->bus(BUSRD);
				for(int i=0;i<num_processors;i++) {
					if(i!=p) {
						ret_val = cacheArray[i]->mesi_busRd(address, line);  //BusRd happens only when the cache is INVALID
						if(ret_val == 0) ++zero_count;
						else if(ret_val == 1) {
							cache_count++;
						} else if(ret_val == 2) {
							illinois = 1;
						}
					}
				}
				if(zero_count == num_processors-1) { 
					cacheArray[p]->mem();
					cacheArray[p]->memCmd();
					cacheArray[p]->memData();
					cacheArray[p]->bus(DATA);
				}

				for(int i=0; i<cache_count; i++) {
					cacheArray[p]->ctoc();
					cacheArray[p]->bus(DATA);
				}
				if(illinois == 1) {
					cacheArray[p]->ctoc();
					cacheArray[p]->bus(DATA);
				}
			}
		} else if(op == 'w') {
			cache_count = 0;
			illinois = 0;

			if(line == NULL || line->getFlags() == VALID) 
				cacheArray[p]->bus(BUSRDX);

			for(int i=0; i<num_processors;i++) {
				if(i!=p) {
					if(line == NULL || line->getFlags() == INVALID) {
						/* Coherence Misses */
						if(cacheArray[p]->checkTag(address) == 1) {
							cacheArray[p]->coherenceMiss();
						}

						ret_val = cacheArray[i]->mesi_busRdX(address);
						if(ret_val == 0) ++zero_count;
						else if(ret_val == 1) { 
							cache_count++;
						} else if(ret_val == 2) {
							illinois = 1;
						}
					} else if(line->getFlags() == VALID) {
						cacheArray[i]->mesi_busUpgr(address);
					}
				}
			}

			if(zero_count == num_processors-1) { 
				cacheArray[p]->mem();
				/*new code begins*/
				cacheArray[p]->memCmd();
				cacheArray[p]->memData();
				/*new code ends*/
				cacheArray[p]->bus(DATA);
			}
			for(int i=0; i<cache_count;i++) {
				cacheArray[p]->ctoc();
				cacheArray[p]->bus(DATA);
			}
			if(illinois == 1) {
				cacheArray[p]->ctoc();
				cacheArray[p]->bus(DATA);
			}
		}
		cacheArray[p]->Access_MESI(address, op, p, cacheArray);
	}
}

void msi(Cache **cacheArray, int num_processors, char *fname) {
    char *buf = (char *)malloc(13*sizeof(char));
    size_t len = 13;
    if((fp = fopen(fname, "r")) == NULL) {
        cout<< "Error opening trace file"<<endl;
        exit(-1);
    }
    
	while((getline(&buf, &len, fp))!= -1) {
		int zero_count = 0;
        int ret_val = sscanf(buf, "%d %c %x", &p, &op, &address);
		if(ret_val == -1) {
			printf("sscanf failed\n");
			exit(0);
		}

		cacheLine *line = cacheArray[p]->findLine(address);
		if(op == 'r') {
			if(line == NULL || line->getFlags() == INVALID) {
				/*Coherence Misses */
				/*if(cacheArray[p]->checkTag(address) == 1) {
					cacheArray[p]->coherenceMiss();
				}*/
				cacheArray[p]->bus(BUSRD);
				for(int i=0;i<num_processors; i++) {
					if(i!=p) {
						ret_val = cacheArray[i]->msi_busRd(address);
						//if(ret_val == 0) zero_count++;
					}
				}
				//if(zero_count == num_processors - 1) {
					cacheArray[p]->mem();
					cacheArray[p]->memCmd();
					cacheArray[p]->memData();
					cacheArray[p]->bus(DATA);
				//}
			}
		} else if (op == 'w') {
			/*Coherence Misses */
			/*if(line == NULL) {
				if(cacheArray[p]->checkTag(address)) {
					cacheArray[p]->coherenceMiss();
				}
			}*/
			if(line == NULL || line->getFlags() == VALID) {
				cacheArray[p]->bus(BUSRDX);
				for(int i=0;i<num_processors; i++) {
					if(i!=p) {
						ret_val = cacheArray[i]->msi_busRdX(address);
						if(ret_val == 0) zero_count++;
					}
				}
				
				//if(zero_count == num_processors - 1) {
					cacheArray[p]->mem();
					cacheArray[p]->memCmd();
					cacheArray[p]->memData();
					cacheArray[p]->bus(DATA);
				//}
			}	
		}
		cacheArray[p]->Access(address, op);
    }
}

int main(int argc, char *argv[])
{
	
	ifstream fin;
	//FILE * pFile;

	if(argv[1] == NULL){
		 cout<<"input format: ";
		 cout<<"./smp_cache <cache_size> <assoc> <block_size> <num_processors> <protocol> <trace_file> \n";
		 exit(0);
        }

	/*****uncomment the next five lines*****/
	int cache_size = atoi(argv[1]);
	int cache_assoc= atoi(argv[2]);
	int blk_size   = atoi(argv[3]);
	int num_processors = atoi(argv[4]);/*1, 2, 4, 8*/
	int protocol   = atoi(argv[5]);	 /*0:MSI, 1:MESI, 2:MOESI*/
	char *fname =  (char *)malloc(20);
 	fname = argv[6];

	//char *ccp = (char*) malloc(5*sizeof(char));
	string ccp;
	if(protocol == 0) ccp = "MSI";
	if(protocol == 1) ccp = "MESI";
	if(protocol == 2) ccp = "MOESI";	
	cout<<"===== 506 SMP Simulator Configuration =====";
	cout<<endl<<"L1_SIZE:\t\t\t"<<cache_size<<endl;
	cout<<"L1_ASSOC:\t\t\t"<<cache_assoc<<endl;
	cout<<"L1_BLOCKSIZE:\t\t\t"<<blk_size<<endl;
	cout<<"NUMBER OF PROCESSORS:\t\t"<<num_processors<<endl;
	cout<<"COHERENCE PROTOCOL:\t\t"<<ccp<<endl;
	cout<<"TRACE FILE:\t\t\t./"<<fname<<endl;
 
	//*********************************************//
    //*****create an array of caches here**********//
	//*********************************************//

	Cache** cacheArray = new Cache*[num_processors];
	for(int i=0; i<num_processors; i++) {
		cacheArray[i] = new Cache(cache_size, cache_assoc, blk_size);
	}
	/*Cache c0(cache_size, cache_assoc, blk_size); 
	Cache c1(cache_size, cache_assoc, blk_size);
	Cache c2(cache_size, cache_assoc, blk_size);
	Cache c3(cache_size, cache_assoc, blk_size);
	*/

	/*pFile = fopen (fname,"r");
	if(pFile == 0)
	{   
		cout<<"Trace file problem\n";
		exit(0);
	}*/

	switch(protocol) {
		case 0: 
			msi(cacheArray, num_processors, fname);
			break;
		case 1:
			mesi(cacheArray, num_processors, fname);
			break;
		case 2:
			moesi(cacheArray, num_processors, fname);
			break;
		case 3:
			custom(cacheArray, num_processors, fname);
		default:
			break;
	}

	///******************************************************************//
	//**read trace file,line by line,each(processor#,operation,address)**//
	//*****propagate each request down through memory hierarchy**********//
	//*****by calling cachesArray[processor#]->Access(...)***************//
	///******************************************************************//
	//fclose(pFile);

	//********************************//
	//print out all caches' statistics //
	//********************************//
	
	for(int i=0;i<num_processors;i++) {
		cout<<"===== Simulation results (Cache_"<<i<<")"<<     "      =====\n";
		cacheArray[i]->printStats();
	}
	/*cout<<"===== Simulation results (Cache_"<<0<<")"<<     "      =====\n";
	cacheArray[0]->printStats();
	cout<<"===== Simulation results (Cache_"<<1<<")"<<     "      =====\n";
	cacheArray[1]->printStats();
	cout<<"===== Simulation results (Cache_"<<2<<")"<<     "      =====\n";
	cacheArray[2]->printStats();
	cout<<"===== Simulation results (Cache_"<<3<<")"<<     "      =====\n";
	cacheArray[3]->printStats();*/
}
