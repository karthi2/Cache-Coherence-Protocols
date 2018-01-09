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
	cacheMisses = coherenceMisses = data_mem = cmd_mem = mem_count = busTr = 0;
 
	head = NULL;

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

int Cache::copy_1(ulong addr, int p, Cache **cacheArray) {
	int retval = 0;
	for(int i=0;i<4; i++) {
		if(i!=p) {
            cacheLine *line = cacheArray[i]->findLine(addr);
            if(line!=NULL) {
				line->setFlags(OWNER);
                retval = 1;
			}
        }
    }
    return retval;
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

int Cache::isCopyShared(ulong addr, Cache **cacheArray) {
	int count = 0;
	for(int i=0;i<4;i++) {
		if(this!=cacheArray[i]) {
			cacheLine *line = cacheArray[i]->findLine_custom(addr);
			if(line == NULL) {}
			else {
				if(count == 1) {
					line->setFlags(VALID);
					continue;
				} 
				line->setFlags(OWNER);
				count = 1;
				//return 1;
			}
		}
	}
	return count;
	//return 0;
}

void Cache::Access_custom(ulong addr, uchar op, int p, Cache **cacheArray) {
	currentCycle++;

	if(op == 'w') writes++;
	else reads++;

	cacheLine *line = findLine(addr);
	if(line == NULL) {  //MISS
		if(op == 'w') writeMisses++;
		else readMisses++;
		cacheMisses++;

		cacheLine *newLine = fillLine(addr, p, cacheArray);
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
		}
	}
}

void Cache::Access_MOESI(ulong addr, uchar op, int p, Cache **cacheArray) {
	currentCycle++;

	if(op == 'w') {
		writes++;
	} else {
		reads++;
	}

	cacheLine *line = findLine(addr);
	if(line == NULL) {  //MISS
		if(op == 'w') writeMisses++;
		else readMisses++;
		cacheMisses++;

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
		}
	}
}

void Cache::Access_MESI(ulong addr, uchar op, int p, Cache **cacheArray) {
	currentCycle++;
	
	if(op == 'w') writes++;
	else reads++;

	cacheLine *line = findLine(addr);
	if(line == NULL || line->getFlags() == INVALID) {   //MISS
		if(op == 'w') writeMisses++;
		else readMisses++;
		
		cacheMisses++;

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
	else reads++;
	
	cacheLine *line = findLine(addr);
	if(line == NULL || line->getFlags() == INVALID)/*miss*/
	{
		if(checkTag(addr)) 
			coherenceMisses++;

		if(op == 'w') writeMisses++;
		else readMisses++;
		
		cacheMisses++;

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

cacheLine * Cache::findLine_custom(ulong addr)
{
   ulong i, j, tag, pos;
   
   pos = assoc;
   tag = calcTag(addr);
   i   = calcIndex(addr);
  
   for(j=0; j<assoc; j++)
		if(cache[i][j].getTag() == tag)
		{
		     pos = j; break; 
		}
   if(pos == assoc)
	return NULL;
   else
	return &(cache[i][pos]); 
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

/*void isCopyShared(ulong addr, Cache **cacheArray) {
	int ret_val = 0;
	for(int i=0;i<4;i++) {
		if(this!=CacheArray[i]) {
			cacheLine *line = findLine(addr);
			if(line == NULL) {}
			else if(line->getFlags() == SHARED) {
				line->setFlags(OWNER);
				break;
			}
		}
	}
}*/

/*allocate a new line*/
cacheLine *Cache::fillLine(ulong addr, int p, Cache **cacheArray)
{ 
   ulong tag;
  
   cacheLine *victim = findLineToReplace(addr);
   assert(victim != 0);

   int status = 0;

	/*if(victim->getFlags() == OWNER) {
		p = isCopyShared(addr, cacheArray);
	}*/

	if(victim->getFlags() == DIRTY) {
		status  = isCopyShared(addr, cacheArray);
		this->flushes++;
	} else if(victim->getFlags() == OWNER) {
		status = copy_1(addr, p, cacheArray);
	}


  /* if(victim->getFlags() == OWNER || victim->getFlags() == DIRTY) {
		p = isCopyShared(addr, cacheArray);
		if(victim->getFlags() == DIRTY) this->flushes++;
   }*/

   if(status==0 && (victim->getFlags() == DIRTY || victim->getFlags() == OWNER)) {
			writeBack(addr);
   }

   tag = calcTag(addr);   
   victim->setTag(tag);
   victim->setFlags(VALID);    
   /**note that this cache line has been already 
      upgraded to MRU in the previous function (findLineToReplace)**/

   return victim;
}

cacheLine *Cache::fillLine(ulong addr)
{ 
   ulong tag;
  
   cacheLine *victim = findLineToReplace(addr);
   assert(victim != 0);
   if(victim->getFlags() == DIRTY || victim->getFlags() == OWNER) writeBack(addr);
   
   //if(victim->getFlags() == INVALID) {
		int status = checkTag(addr);
   //}

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
		cout<<"19"<<". number of cache misses:\t\t"<<cacheMisses<<endl;
		cout<<"20"<<". number of coherence misses:\t\t"<<coherenceMisses<<endl;
		cout<<"21"<<". number of bus transactions:\t\t"<<busTr<<endl;
		cout<<"22"<<". number of mem transactions:\t\t"<<mem_count<<endl;
		cout<<"23"<<". number of commands to memory:\t\t"<<cmd_mem<<endl;
		cout<<"24"<<". number of data to/from memory:\t\t"<<data_mem<<endl;
}

void Cache::addToList(TAG t) {
	TAG temp = NULL;
	if(head == NULL) {
		head = t;
		head->next = NULL;
		return;
	} else {
		temp = head;
		while(head->next!=NULL) {
			head = head->next;
		}
		head->next = t;
		head->next->next = NULL;
	}
	head = temp;
}

int Cache::checkTag(ulong addr) {
	ulong tag = calcTag(addr);
	ulong index = calcIndex(addr);

	if(head == NULL) return 0;
	TAG temp = head->next;
	TAG prev = head;

	/* Head node matches */
	if(tag == head->tag && index == head->index) {
		//temp = head->next;
		free(head);
		head = temp;
		return 1;
	}

	while(temp!=NULL) {
		if(tag == temp->tag && index == temp->index) {
			prev->next = temp->next;
			free(temp);
			return 1;
		}
		prev = temp;
		temp = temp->next;
	}
	return 0;
}

int Cache::msi_busRd(ulong addr) {
	//find whether the block requested is in its cache
	cacheLine *line = findLine(addr);
	if(line == NULL || line->getFlags() == INVALID) {
		return 0;
		//this cache is in invalid state. Do nothing
	}
	if(line->getFlags() == DIRTY) {
		line->setFlags(VALID);
		++this->m2s;
		++this->flushes;
		++this->interventions;
		
		++this->mem_count;
		++this->data_mem;
		this->bus(DATA);
	} /*else if(line->getFlags == VALID) {
		++this->mem_count;
		++this->cmd_mem;
		++this->data_mem;
		this->bus(DATA);
	}*/
	return 0;
}

int Cache::msi_busRdX(ulong addr) {
	//find whether the block requested is in its cache
	cacheLine *line = findLine(addr);
	if(line == NULL || line->getFlags() == INVALID) {
		return 0;
	}
	if(line->getFlags() == DIRTY) {
		++this->flushes;
		this->mem();
		++this->data_mem;
		this->bus(DATA);
	} /*else if(line->getFlags() == VALID) {
		++this->mem_count;
		++this->cmd_mem;
		++this->data_mem;
		this->bus(DATA);
	}*/ 

	line->setFlags(INVALID);
	++this->invalidations;

	ulong tag = calcTag(addr);
	ulong index = calcIndex(addr);
	TAG t = (struct tag_info *) malloc (sizeof(struct tag_info));
	t->tag = tag;
	t->index = index;
	t->next = NULL;
	this->addToList(t);

	return 0;
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
		this->mem();
		this->memData();
		++this->m2s;
		++this->interventions;
		line->setFlags(VALID);
		return 1;
	} else if(line->getFlags() == VALID) {
		return 2;
	}
	//return 0;
}

int Cache::mesi_busRdX(ulong addr) {
	cacheLine *line = findLine(addr);

	if(line == NULL || line->getFlags() == INVALID) {
		return 0;
	}
	
	//Add the tag to the list
	ulong tag = calcTag(addr);
	ulong index = calcIndex(addr);
	TAG t = (struct tag_info *) malloc (sizeof(struct tag_info));
	t->tag = tag;
	t->index = index;
	t->next = NULL;
	this->addToList(t);
	
	if(line->getFlags() == EXCLUSIVE) {
		++this->invalidations;
		line->setFlags(INVALID);
		return 1;
	} else if(line->getFlags() == VALID) {
		++this->invalidations;
		line->setFlags(INVALID);
		return 2;
	} else if(line->getFlags() == DIRTY) {
		++this->invalidations;
		++this->flushes;
		this->mem();
		this->memData();
		line->setFlags(INVALID);
		return 1;
	}
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
		//Add the tag to the list
		ulong tag = calcTag(addr);
		ulong index = calcIndex(addr);
		TAG t = (struct tag_info *) malloc (sizeof(struct tag_info));
		t->tag = tag;
		t->index = index;
		t->next = NULL;
		this->addToList(t);
		
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
		//this->bus(DATA);
	} 
	//Add the tag to the list
	ulong tag = calcTag(addr);
	ulong index = calcIndex(addr);
	TAG t = (struct tag_info *) malloc (sizeof(struct tag_info));
	t->tag = tag;
	t->index = index;
	t->next = NULL;
	this->addToList(t);
	
	++this->invalidations;
	line->setFlags(INVALID);
	return 1;
}

void Cache::moesi_busUpgr(ulong addr) {
	cacheLine *line = findLine(addr);

    if(line == NULL || line->getFlags() == INVALID) {
        return;
    }

	//Add the tag to the list
	ulong tag = calcTag(addr);
	ulong index = calcIndex(addr);
	TAG t = (struct tag_info *) malloc (sizeof(struct tag_info));
	t->tag = tag;
	t->index = index;
	t->next = NULL;
	this->addToList(t);
	
	//Actually nobody does anything on busUpgr
	line->setFlags(INVALID);
	++this->invalidations;
}

int Cache::custom_busRd(ulong addr) {
	cacheLine *line = findLine(addr);
	if(line == NULL || line->getFlags() == INVALID) {
		return 0;
	} else if(line->getFlags() == EXCLUSIVE) {
		line->setFlags(OWNER);   //Move to owner instead of shared
		//++this->e2s;
		return 1;
	} else if(line->getFlags() == DIRTY) {
		line->setFlags(OWNER);
		++this->flushes;
		++this->m2o;
		++this->interventions;
		//this->bus(DATA);
		return 1;
	} else if(line->getFlags() == OWNER) {
		++this->flushes;
		//this->bus(DATA);
		return 1;
	}
}

int Cache::custom_busRdX(ulong addr) {
	cacheLine *line = findLine(addr);

	if(line == NULL || line->getFlags() == INVALID || line->getFlags() == VALID) {
        return 0;
    }
	if(line->getFlags() == DIRTY || line->getFlags() == OWNER) {
		++this->flushes;
		//this->bus(DATA);
	} 
	//Add the tag to the list
	ulong tag = calcTag(addr);
	ulong index = calcIndex(addr);
	TAG t = (struct tag_info *) malloc (sizeof(struct tag_info));
	t->tag = tag;
	t->index = index;
	t->next = NULL;
	this->addToList(t);
	
	++this->invalidations;
	line->setFlags(INVALID);
	return 1;
}

void Cache::custom_busUpgr(ulong addr) {
	cacheLine *line = findLine(addr);

    if(line == NULL || line->getFlags() == INVALID) {
        return;
    }

	//Add the tag to the list
	ulong tag = calcTag(addr);
	ulong index = calcIndex(addr);
	TAG t = (struct tag_info *) malloc (sizeof(struct tag_info));
	t->tag = tag;
	t->index = index;
	t->next = NULL;
	this->addToList(t);
	
	//Actually nobody does anything on busUpgr
	line->setFlags(INVALID);
	++this->invalidations;
}
