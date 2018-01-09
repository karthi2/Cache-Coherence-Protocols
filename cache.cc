/*******************************************************
                          cache.cc
                  Ahmad Samih & Yan Solihin
                           2009
                {aasamih,solihin}@ece.ncsu.edu
********************************************************/

#include <stdlib.h>
#include <assert.h>
#include "cache.h"
#include <iostream>
#include <stdio.h>
using namespace std;

Cache::Cache(int s,int a,int b )
{
	ulong i, j;
   	reads = readMisses = writes = 0; 
   	writeMisses = writeBacks = currentCycle = 0;

	size       = (ulong)(s);
   	lineSize   = (ulong)(b);
	assoc      = (ulong)(a);   
   	sets       = (ulong)((s/b)/a);
   	numLines   = (ulong)(s/b);
   	log2Sets   = (ulong)(log2(sets));   
   	log2Blk    = (ulong)(log2(b));   
  
   	//*******************//
   	//initialize your counters here//
   	//*******************//

	flushes = invalidations = interventions = c2c = m2o = o2m = e2m = i2m = s2m = e2s = m2s = i2s = i2e = 0;
 
   tagMask =0;
   for(i=0;i<log2Sets;i++)
   {
		tagMask <<= 1;
        tagMask |= 1;
   }
   
   /**create a two dimentional cache, sized as cache[sets][assoc]**/ 
   cache = new cacheLine*[sets];
   for(i=0; i<sets; i++)
   {
      cache[i] = new cacheLine[assoc];
      for(j=0; j<assoc; j++) 
      {
	   cache[i][j].invalidate();
      }
   }
}

int Cache::copy(ulong addr, int p, Cache **cacheArray) {
	int retval = 0;
	for(int i=0;i<4; i++) {
		if(i!=p) {
            cacheLine *line = cacheArray[i]->findLine(addr);
            if(line!=NULL)
                retval = 1;
        }
    }
    return retval;
}

void Cache::Access_MOESI(ulong addr, uchar op, int p, Cache **cacheArray) {
	currentCycle++;

	if(op == 'w') writes++;
	else reads++;

	cacheLine *line = findLine(addr);
	if(line == NULL) {  //MISS
		if(op == 'w') writeMisses++;
		else readMisses++;

		cacheLine *newLine = fillLine(addr);
		if(op == 'w') {
			newLine->setFlags(DIRTY);
			i2m++;
		} else {
			if(!copy(addr, p, cacheArray)) {
				newLine->setFlags(EXCLUSIVE);
				i2e++;
			} else {
				newLine->setFlags(VALID);
				i2s++;
			}
		}
	} else {   //HIT
		updateLRU(line);
		if(op == 'w') {
			if(line->getFlags() == EXCLUSIVE) {
				e2m++;
			} else if(line->getFlags() == VALID) {
				s2m++;
			} else if(line->getFlags() == OWNER) {
				o2m++;
			}
			line->setFlags(DIRTY);
		} else {
			//Its a read... no-op
		}
	}
}

void Cache::Access_MESI(ulong addr, uchar op, int p, Cache **cacheArray) {
	currentCycle++;
	
	if(op == 'w') {
		writes++;
	} else {
		reads++;
	}

	cacheLine *line = findLine(addr);
	if(line == NULL || line->getFlags() == INVALID) {   //MISS
		if(op == 'w') writeMisses++;
		else readMisses++;

		cacheLine *newLine = fillLine(addr);
		if(op == 'w') {
			newLine->setFlags(DIRTY);
			i2m++;
		} else {
			if(copy(addr, p, cacheArray) == 0) {
				newLine->setFlags(EXCLUSIVE);
				i2e++;
			} else {
				newLine->setFlags(VALID);
				i2s++;
			}
		}
	} else { //HIT
		updateLRU(line);
		if(line->getFlags() == VALID) {
			if(op == 'w') {
				s2m++;
				line->setFlags(DIRTY);
			}
		} else if(line->getFlags() == EXCLUSIVE) {
			if(op == 'w') {
				e2m++;
				line->setFlags(DIRTY);
			}
		} else if(line->getFlags() == DIRTY) {
			//DO NOTHING
		}
	}
}

/**you might add other parameters to Access()
since this function is an entry point 
to the memory hierarchy (i.e. caches)**/
void Cache::Access(ulong addr,uchar op)
{
	currentCycle++;/*per cache global counter to maintain LRU order 
			among cache ways, updated on every cache access*/
        	
	if(op == 'w') writes++;
	else          reads++;
	
	cacheLine * line = findLine(addr);
	if(line == NULL || line->getFlags() == INVALID)/*miss*/
	{
		if(op == 'w') writeMisses++;
		else readMisses++;

		cacheLine *newline = fillLine(addr);
   		if(op == 'w') {
			newline->setFlags(DIRTY);
			i2m++;
		} else {
			newline->setFlags(VALID);
			i2s++;
		}
		
	}
	else
	{
		/**since it's a hit, update LRU and update dirty flag**/
		updateLRU(line);
		if(line->getFlags() == VALID) {
			if(op == 'w') {
				line->setFlags(DIRTY);
				s2m++;
			}
		}
	}
}

/*look up line*/
cacheLine * Cache::findLine(ulong addr)
{
   ulong i, j, tag, pos;
   
   pos = assoc;
   tag = calcTag(addr);
   i   = calcIndex(addr);
  
   for(j=0; j<assoc; j++)
	if(cache[i][j].isValid())
	        if(cache[i][j].getTag() == tag)
		{
		     pos = j; break; 
		}
   if(pos == assoc)
	return NULL;
   else
	return &(cache[i][pos]); 
}

/*upgrade LRU line to be MRU line*/
void Cache::updateLRU(cacheLine *line)
{
  line->setSeq(currentCycle);  
}

/*return an invalid line as LRU, if any, otherwise return LRU line*/
cacheLine * Cache::getLRU(ulong addr)
{
   ulong i, j, victim, min;

   victim = assoc;
   min    = currentCycle;
   i      = calcIndex(addr);
   
   for(j=0;j<assoc;j++)
   {
      if(cache[i][j].isValid() == 0) return &(cache[i][j]);     
   }   
   for(j=0;j<assoc;j++)
   {
	 if(cache[i][j].getSeq() <= min) { victim = j; min = cache[i][j].getSeq();}
   } 
   assert(victim != assoc);
   
   return &(cache[i][victim]);
}

/*find a victim, move it to MRU position*/
cacheLine *Cache::findLineToReplace(ulong addr)
{
   cacheLine * victim = getLRU(addr);
   updateLRU(victim);
  
   return (victim);
}

/*allocate a new line*/
cacheLine *Cache::fillLine(ulong addr)
{ 
   ulong tag;
  
   cacheLine *victim = findLineToReplace(addr);
   assert(victim != 0);
   if(victim->getFlags() == DIRTY || victim->getFlags() == OWNER) writeBack(addr);
    	
   tag = calcTag(addr);   
   victim->setTag(tag);
   victim->setFlags(VALID);    
   /**note that this cache line has been already 
      upgraded to MRU in the previous function (findLineToReplace)**/

   return victim;
}

void Cache::printStats()
{ 
	/****print out the rest of statistics here.****/
	/****follow the ouput file format**************/
		cout<<"01"<<". number of reads:\t\t\t\t"<<reads<<endl;
		cout<<"02"<<". number of read misses:\t\t\t"<<readMisses<<endl;
		cout<<"03"<<". number of writes:\t\t\t\t"<<writes<<endl;
		cout<<"04"<<". number of write misses:\t\t\t"<<writeMisses<<endl;
		cout<<"05"<<". number of write backs:\t\t\t"<<writeBacks<<endl;
		cout<<"06"<<". number of invalid to exclusive (INV->EXC):\t"<<i2e<<endl;
		cout<<"07"<<". number of invalid to shared (INV->SHD):\t"<<i2s<<endl;
		cout<<"08"<<". number of modified to shared (MOD->SHD):\t"<<m2s<<endl;
		cout<<"09"<<". number of exclusive to shared (EXC->SHD):\t"<<e2s<<endl;
		cout<<"10"<<". number of shared to modified (SHD->MOD):\t"<<s2m<<endl;
		cout<<"11"<<". number of invalid to modified (INV->MOD):\t"<<i2m<<endl;
		cout<<"12"<<". number of exclusive to modified (EXC->MOD):\t"<<e2m<<endl;
		cout<<"13"<<". number of owned to modified (OWN->MOD):\t"<<o2m<<endl;
		cout<<"14"<<". number of modified to owned (MOD->OWN):\t"<<m2o<<endl;
		cout<<"15"<<". number of cache to cache transfers:\t\t"<<c2c<<endl;
		cout<<"16"<<". number of interventions:\t\t\t"<<interventions<<endl;
		cout<<"17"<<". number of invalidations:\t\t\t"<<invalidations<<endl;
		cout<<"18"<<". number of flushes:\t\t\t\t"<<flushes<<endl;
}

void Cache::msi_busRd(ulong addr) {
	//find whether the block requested is in its cache
	cacheLine *line = findLine(addr);
	if(line == NULL || line->getFlags() == INVALID) {
		return;
		//this cache is in invalid state. Do nothing
	}
	if(line->getFlags() == DIRTY) {
		line->setFlags(VALID);
		++this->m2s;
		++this->flushes;
		++this->interventions;
	}
}

void Cache::msi_busRdX(ulong addr) {
	//find whether the block requested is in its cache
	cacheLine *line = findLine(addr);
	if(line == NULL || line->getFlags() == INVALID) {
		return;
		//this cache is in invalid state. Do nothing
	}
	if(line->getFlags() == DIRTY) {
		++this->flushes;
	} else if(line->getFlags() == VALID) {
		//no-op
	}
	line->setFlags(INVALID);
	++this->invalidations;
}

int Cache::mesi_busRd(ulong addr, cacheLine *incoming) {
	cacheLine *line = findLine(addr);
	if(line == NULL || line->getFlags() == INVALID) {
		return 0;
	}
	if(line->getFlags() == EXCLUSIVE) {
		++this->e2s;
		line->setFlags(VALID);
		return 1;
	} else if(line->getFlags() == DIRTY) {
		++this->flushes;
		++this->m2s;
		++this->interventions;
		//++this->writeBacks;
		line->setFlags(VALID);
		return 1;
	} else if(line->getFlags() == VALID) {
		return 2;
	}
	return 0;
}

int Cache::mesi_busRdX(ulong addr) {
	cacheLine *line = findLine(addr);

	if(line == NULL || line->getFlags() == INVALID) {
		return 0;
	}
	if(line->getFlags() == EXCLUSIVE) {
		++this->invalidations;
		line->setFlags(INVALID);
		return 1;
	} else if(line->getFlags() == VALID) {
		++this->invalidations;
	} else if(line->getFlags() == DIRTY) {
		++this->invalidations;
		++this->flushes; //Definite flush
		//++this->writeBacks;
		//printf("Definite flush\n");
		line->setFlags(INVALID);
		return 1;
	}
	line->setFlags(INVALID);
	return 0;
}

void Cache::mesi_busUpgr(ulong addr) {
	cacheLine *line = findLine(addr);

	if(line == NULL || line->getFlags() == INVALID) {
		return;
	}
	if(line->getFlags() == DIRTY || line->getFlags() == EXCLUSIVE) {
		printf("This should never happen - coexistence of shared and exclusive/modified state\n");
		exit(0);
	}
	if(line->getFlags() == VALID) {
		++this->invalidations;
		line->setFlags(INVALID);
	}
}

int Cache::moesi_busRd(ulong addr) {
	cacheLine *line = findLine(addr);

	if(line == NULL || line->getFlags() == INVALID) {
        return 0;
    } else if(line->getFlags() == EXCLUSIVE) {
		line->setFlags(VALID);
		++this->e2s;
		return 1;
	} else if(line->getFlags() == DIRTY) {
		line->setFlags(OWNER);
		++this->flushes;
		++this->m2o;
		++this->interventions;
		return 1;
	} else if(line->getFlags() == OWNER) {
		++this->flushes;
		return 1;
	}
}

int Cache::moesi_busRdX(ulong addr) {
	cacheLine *line = findLine(addr);

	if(line == NULL || line->getFlags() == INVALID || line->getFlags() == VALID) {
        return 0;
    }
	if(line->getFlags() == DIRTY || line->getFlags() == OWNER) {
		++this->flushes;
	} 
	++this->invalidations;
	line->setFlags(INVALID);
	return 1;
}

void Cache::moesi_busUpgr(ulong addr) {
	cacheLine *line = findLine(addr);

    if(line == NULL || line->getFlags() == INVALID) {
        return;
    }

	//Actually nobody does anything on busUpgr
	line->setFlags(INVALID);
	++this->invalidations;
}
