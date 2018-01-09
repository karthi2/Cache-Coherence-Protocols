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
enum{
	INVALID = 0,
	VALID,
	DIRTY,
	EXCLUSIVE,
	OWNER
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

		Cache() { }     
	    Cache(int,int,int);
	   ~Cache() { 
			delete cache;
		}
  
	   	cacheLine *findLineToReplace(ulong addr);
	   	cacheLine *fillLine(ulong addr);
	   	cacheLine *findLine(ulong addr);
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

		void Access(ulong, uchar);
		void Access_MESI(ulong, uchar, int, Cache **);
		void Access_MOESI(ulong, uchar, int, Cache **);
		void printStats();
		void updateLRU(cacheLine *);

		void msi_busRd(ulong addr);
		void msi_busRdX(ulong addr);

		int mesi_busRd(ulong addr, cacheLine *incoming);
		int  mesi_busRdX(ulong addr);
		void mesi_busUpgr(ulong addr);
		int copy(ulong, int, Cache**);

		int moesi_busRd(ulong);
		int moesi_busRdX(ulong);
		void moesi_busUpgr(ulong);
	   	//******///
	   	//add other functions to handle bus transactions///
	   	//******///
};

#endif
