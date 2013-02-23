/*
* (C) Iain Fraser - GPLv3 
*
* 1:1 mapping of LuaVM instructions
*/

#ifndef _INSTRUCTION_H_
#define _INSTRUCTION_H_

#include "mc_emitter.h"

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


void emit_loadk( struct arch_emitter** mce, int l, int k );
void emit_move( struct arch_emitter** mce, loperand d, loperand s );

void emit_add( struct arch_emitter** mce, loperand d, loperand s, loperand t );
void emit_sub( struct arch_emitter** mce, loperand d, loperand s, loperand t );
void emit_mul( struct arch_emitter** mce, loperand d, loperand s, loperand t );
void emit_div( struct arch_emitter** mce, loperand d, loperand s, loperand t );
void emit_mod( struct arch_emitter** mce, loperand d, loperand s, loperand t );

void emit_forprep( struct arch_emitter** mce, loperand init, int pc, int j );
void emit_forloop( struct arch_emitter** mce, loperand loopvar, int pc, int j );

void emit_gettable( struct arch_emitter** mce, loperand dst, loperand table, loperand idx );
void emit_newtable( struct arch_emitter** mce, loperand dst, int array, int hash );
void emit_setlist( struct arch_emitter** mce, loperand table, int n, int block );

void emit_closure( struct arch_emitter** mce, loperand dst, struct proto* p );
void emit_call( struct arch_emitter** mce, loperand closure, int nr_params, int nr_results );
void emit_ret( struct arch_emitter** mce );

#endif 
