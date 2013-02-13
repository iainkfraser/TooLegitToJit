/*
* (C) Iain Fraser - GPLv3  
* Generic machine code emitter interface.
*/

#ifndef _MC_EMITTER_H_
#define _MC_EMITTER_H_

#include <stddef.h>
#include <stdbool.h>
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

/* machine code emitter control interface */
void mce_init( void** mce, size_t vmlines, size_t nr_constants );

#if 0
void mce_start( void** mce, size_t vmlines );
#else
void mce_start( void** mce, int nr_locals );
#endif

size_t mce_link( void** mce );
void* mce_stop( void** mce, void* buf );
void label( unsigned int pc , void** Dst );

/* 1:1 mapping to LuaVM instructions */
void emit_ret( void** mce );
void emit_loadk( void** mce, int l, int k );
void emit_move( void** mce, loperand d, loperand s );
void emit_add( void** mce, loperand d, loperand s, loperand t );
void emit_sub( void** mce, loperand d, loperand s, loperand t );
void emit_forprep( void** mce, loperand init, int pc, int j );
void emit_forloop( void** mce, loperand loopvar, int pc, int j );
void emit_newtable( void** mce, loperand dst, int array, int hash );
void emit_setlist( void** mce, loperand table, int n, int block );
#endif
