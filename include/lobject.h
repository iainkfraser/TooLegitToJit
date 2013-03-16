/*
* (C) Iain Fraser - GPLv3 
* Lua object definitions  
*/

#ifndef _LOBJECT_H_
#define _LOBJECT_H_

#include <stdint.h>
#include <stdbool.h>
#include "list.h"
#include "macros.h"

typedef uint32_t word;
typedef uintptr_t wordp;
typedef word lua_Number;
typedef word Tag;

union Value {
	// garbage collectable object
	bool		b;
	lua_Number	n;
	// light userdata
	// light C function
};

gl_static_assert( sizeof( union Value ) == sizeof( word ) );
gl_static_assert( sizeof( wordp ) == sizeof( word ) );

struct TValue {
	Tag t;		// compile will word align might as well make it explcit	
	union Value v;
};	// tagged value

struct UpVal {
	struct TValue* val;
	union {
		struct TValue v;
		struct list_head link;
	};
};


struct upval_desc {
	uint8_t	instack;	// in stack or parents upvalues
	uint8_t idx;		// index into stack or parents upvalues
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


struct closure {
	struct proto* p;
	struct UpVal* uvs[];	
};

#endif
