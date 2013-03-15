/*
* (C) Iain Fraser - GPLv3 
* Lua object definitions  
*/

#ifndef _LOBJECT_H_
#define _LOBJECT_H_

#include "list.h"

typedef int TValue;

struct UpVal {
	TValue* val;
	union {
		TValue v;
		struct {
			struct UpVal *prev;
			struct UpVal *next;
		};
	};
};


struct upval_desc {
	uint8_t	instack;	// in stack or parents upvalues
	uint8_t idx;		// index into stack or parents upvalues
};



struct closure {
	struct proto* p;
	uint32_t upval[];	
};


struct proto {
	int linedefined;
	int lastlinedefined;
	int sizecode;
	int sizemcode;
	int sizeupvalues;
	int nrconstants;	
	int nrprotos;
	uint8_t numparams;
	uint8_t is_vararg;
	uint8_t maxstacksize;	
	void*	code;
	void*	code_start;

#ifdef _ELFDUMP_
	int	strtabidx;	// string table index
	int	secoff;
	int	secend;
#endif

	struct proto *subp;	// child prototypes 
	struct upval_desc *uvd;
};

#endif
