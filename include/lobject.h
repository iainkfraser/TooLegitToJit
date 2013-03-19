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

/*
** basic types
*/
#define LUA_TNONE		(-1)

#define LUA_TNIL		0
#define LUA_TBOOLEAN		1
#define LUA_TLIGHTUSERDATA	2
#define LUA_TNUMBER		3
#define LUA_TSTRING		4
#define LUA_TTABLE		5
#define LUA_TFUNCTION		6
#define LUA_TUSERDATA		7
#define LUA_TTHREAD		8

#define LUA_NUMTAGS		9

/*
** tags for Tagged Values have the following use of bits:
** bits 0-3: actual tag (a LUA_T* value)
** bits 4-5: variant bits
** bit 6: whether value is collectable
*/

#define VARBITS		(3 << 4)


/*
** LUA_TFUNCTION variants:
** 0 - Lua function
** 1 - light C function
** 2 - regular C function (closure)
*/

/* Variant tags for functions */
#define LUA_TLCL	(LUA_TFUNCTION | (0 << 4))  /* Lua closure */
#define LUA_TLCF	(LUA_TFUNCTION | (1 << 4))  /* light C function */
#define LUA_TCCL	(LUA_TFUNCTION | (2 << 4))  /* C closure */


/*
** LUA_TSTRING variants */
#define LUA_TSHRSTR	(LUA_TSTRING | (0 << 4))  /* short strings */
#define LUA_TLNGSTR	(LUA_TSTRING | (1 << 4))  /* long strings */


/* Bit mark for collectable types */
#define BIT_ISCOLLECTABLE	(1 << 6)

/* mark a tag as collectable */
#define ctb(t)			((t) | BIT_ISCOLLECTABLE)

#define tag( o )			( (o)->t )
#define tag_gentype( o )		( tag( o ) & 0x0f ) 
#define tag_spectypevariant( o )	( tag( o ) & 0x3f )

struct table;
struct closure;
struct lua_State;

typedef uint32_t word;
typedef uintptr_t wordp;
typedef word lua_Number;
typedef word Tag;
typedef int (*lua_CFunction) ( struct lua_State *L );

struct gcheader {
	int rc;	
};

union Value {
	struct gcheader *gc;	// garbage collectable object
	bool		b;	// boolean
	lua_Number	n;	// integer
	lua_CFunction	f;	// light C function
	void*		p;	// light user data
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
	struct gcheader h;
	struct proto* p;
	struct UpVal* uvs[];	
};

typedef struct table {
	struct gcheader h;	
	size_t asz;		// array size	
	struct dictionary *d;
	struct TValue array[];
} table_t; 

#endif
