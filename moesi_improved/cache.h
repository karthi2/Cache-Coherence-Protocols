/*******************************************************
                          cache.h
                  Ahmad Samih & Yan Solihin
                           2009
                {aasamih,solihin}@ece.ncsu.edu
********************************************************/

#ifndef CACHE_H
#define CACHE_H

#include <cmath>
#include <iostream>

typedef unsigned long ulong;
typedef unsigned char uchar;
typedef unsigned int uint;

/****add new states, based on the protocol****/
enum {
	INVALID = 0,
	VALID,
	DIRTY,
	EXCLUSIVE,
	OWNER
};

enum TRNS {
	BUSRD = 0,
	BUSRDX,
	DATA
};

typedef struct tag_info *TAG;
struct tag_info {
	ulong tag;
	ulong index;
	TAG next;
};

class cacheLine 
{
	protected:
		ulong tag;
	   	ulong Flags;   // 0:invalid, 1:valid, 2:dirty 
	   	ulong seq; 
 
	public:
		cacheLine() { 
			tag = 0; Flags = 0; 
		}
   		ulong getTag() { 
			return tag; 
		}
   		ulong getFlags() { 
			return Flags;
		}
   		ulong getSeq() { 
			return seq; 
		}
   		void setSeq(ulong Seq) { 
			seq = Seq;
		}
   		void setFlags(ulong flags) {  
			Flags = flags;
		}
   		void setTag(ulong a) { 
			tag = a; 
		}
   		void invalidate() { 
			tag = 0; Flags = INVALID; 
		}
		//useful function
   		bool isValid() { 
			return ((Flags) != INVALID); 
		}
};

class Cache
{
	protected:
   		
		ulong size, lineSize, assoc, sets, log2Sets, log2Blk, tagMask, numLines;
   		ulong reads,readMisses,writes,writeMisses,writeBacks;

		ulong flushes, invalidations, interventions, c2c, m2o, o2m, e2m, i2m, s2m, e2s, m2s, i2s, i2e;
		ulong cacheMisses, coherenceMisses;
		ulong busTr, cmd_mem, data_mem;
		ulong mem_count;
		TAG head;

	   	//******///
	   	//add coherence counters here///
	   	//******///

	   	cacheLine **cache;
		ulong calcTag(ulong addr) { 
			return (addr >> (log2Blk));
		}
	   	ulong calcIndex(ulong addr) { 
			return ((addr >> log2Blk) & tagMask);
		}
	   	ulong calcAddr4Tag(ulong tag) { 
			return (tag << (log2Blk));
		}
   
	public:
    	ulong currentCycle;  

	    Cache(int,int,int);
	   ~Cache() { 
			delete cache;
		}
  
	   	cacheLine *findLineToReplace(ulong addr);
	   	cacheLine *fillLine(ulong addr);
		cacheLine *fillLine(ulong, int, Cache **);
	   	cacheLine *findLine(ulong addr);
	   	cacheLine *findLine_custom(ulong addr);
	   	cacheLine *getLRU(ulong);
   
	   	ulong getRM() {
			return readMisses;
		} 
		ulong getWM() {
			return writeMisses;
		} 
	   	ulong getReads() {
			return reads;
		}
		ulong getWrites() {
			return writes;
		}
	   	ulong getWB() {
			return writeBacks;
		}
   
	   	void writeBack(ulong)   {writeBacks++;}
		void itos() {i2s++;}
		void stom() {s2m++;}
		void itom() {i2m++;}
		void itoe() {i2e++;}
		void ctoc() {c2c++;}
		void mem() {mem_count++;}
		void bus(TRNS trns) {
			busTr++;
			/*if(trns == BUSRD || trns == BUSRDX)
				cmd_mem++;
			else if(trns == DATA)
				data_mem++;*/
		}
		void memCmd() {cmd_mem++;}
		void memData() {data_mem++;}
		void coherenceMiss() {coherenceMisses++;}

		void Access(ulong, uchar);
		void Access_MESI(ulong, uchar, int, Cache **);
		void Access_MOESI(ulong, uchar, int, Cache **);

		void Access_custom(ulong, uchar, int, Cache **);
		void printStats();
		void updateLRU(cacheLine *);

		int msi_busRd(ulong addr);
		int msi_busRdX(ulong addr);

		int mesi_busRd(ulong addr, cacheLine *incoming);
		int  mesi_busRdX(ulong addr);
		void mesi_busUpgr(ulong addr);
		int copy(ulong, int, Cache**);
		int copy_1(ulong, int, Cache **);
		int isCopyShared(ulong, Cache **);

		int moesi_busRd(ulong);
		int moesi_busRdX(ulong);
		void moesi_busUpgr(ulong);

		int custom_busRd(ulong);
		int custom_busRdX(ulong);
		void custom_busUpgr(ulong);
		
		void addToList(TAG);
		int checkTag(ulong);
	   	//******///
	   	//add other functions to handle bus transactions///
	   	//******///
};

#endif
