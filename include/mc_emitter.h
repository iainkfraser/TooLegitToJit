/*
* (C) Iain Fraser - GPLv3  
* Generic machine code emitter interface. One is used per
* function.
*/

#ifndef _MC_EMITTER_H_
#define _MC_EMITTER_H_

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "lopcodes.h"

typedef struct lvm_operand{	// lua VM operand
	bool	islocal;
	int	index;	
} loperand;

static inline loperand to_loperand( int rk ){
	loperand r;
	r.islocal = !ISK( rk );
	r.index = r.islocal ? rk : INDEXK( rk );  
	return r;
} 

struct proto {
	int linedefined;
	int lastlinedefined;
	int sizecode;
	int sizemcode;
	int nrconstants;	
	int nrprotos;
	uint8_t numparams;
	uint8_t is_vararg;
	uint8_t maxstacksize;	
	void*	code;
	void*	code_start;

	struct proto* subp;	// child prototypes 

};

/* machine code emitter control interface */
void mce_init( void** mce, size_t vmlines );
void mce_const_init( void** mce, size_t nr_constants );
void mce_proto_init( void** mce, size_t nr_protos );
void mce_proto_set( void** mce, int pindex, void* addr );


#if 0
void mce_start( void** mce, size_t vmlines );
#else
void mce_start( void** mce, int nr_locals, int nr_params );
#endif

size_t mce_link( void** mce );
void* mce_stop( void** mce, void* buf );
void label( unsigned int pc , void** Dst );

/* 1:1 mapping to LuaVM instructions */
void emit_loadk( void** mce, int l, int k );
void emit_move( void** mce, loperand d, loperand s );

void emit_add( void** mce, loperand d, loperand s, loperand t );
void emit_sub( void** mce, loperand d, loperand s, loperand t );
void emit_mul( void** mce, loperand d, loperand s, loperand t );
void emit_div( void** mce, loperand d, loperand s, loperand t );
void emit_mod( void** mce, loperand d, loperand s, loperand t );

void emit_forprep( void** mce, loperand init, int pc, int j );
void emit_forloop( void** mce, loperand loopvar, int pc, int j );

void emit_gettable( void** mce, loperand dst, loperand table, loperand idx );
void emit_newtable( void** mce, loperand dst, int array, int hash );
void emit_setlist( void** mce, loperand table, int n, int block );

void emit_closure( void** mce, loperand dst, struct proto* p );
void emit_call( void** mce, loperand closure, int nr_params, int nr_results );
void emit_ret( void** mce );

/* constant loading */
void mce_const_int( void** mce, int kindex, int value );

#endif
