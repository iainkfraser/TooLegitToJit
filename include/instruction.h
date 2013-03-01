/*
* (C) Iain Fraser - GPLv3 
*
* 1:1 mapping of LuaVM instructions
*/

#ifndef _INSTRUCTION_H_
#define _INSTRUCTION_H_

#include "emitter.h"
#include "frame.h"
#include "operand.h"

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


void emit_loadk( struct emitter** mce, struct machine_ops* mop, struct frame* f, int l, int k );
void emit_move( struct emitter** mce, struct machine_ops* mop, struct frame* f, loperand d, loperand s );

void emit_add( struct emitter** mce, struct machine_ops* mop, struct frame* f, loperand d, loperand s, loperand t );
void emit_sub( struct emitter** mce, struct machine_ops* mop, struct frame* f, loperand d, loperand s, loperand t );
void emit_mul( struct emitter** mce, struct machine_ops* mop, struct frame* f, loperand d, loperand s, loperand t );
void emit_div( struct emitter** mce, struct machine_ops* mop, struct frame* f, loperand d, loperand s, loperand t );
void emit_mod( struct emitter** mce, struct machine_ops* mop, struct frame* f, loperand d, loperand s, loperand t );

void emit_forprep( struct emitter** mce, struct machine_ops* mop, struct frame* f, loperand init, int pc, int j );
void emit_forloop( struct emitter** mce, struct machine_ops* mop, struct frame* f, loperand loopvar, int pc, int j );

void emit_gettable( struct emitter** mce, struct machine_ops* mop, struct frame* f, loperand dst, loperand table, loperand idx );
void emit_newtable( struct emitter** mce, struct machine_ops* mop, struct frame* f, loperand dst, int array, int hash );
void emit_setlist( struct emitter** mce, struct machine_ops* mop, struct frame* f, loperand table, int n, int block );

void emit_closure( struct emitter** mce, struct machine_ops* mop, struct frame* f, loperand dst, struct proto* p );
void emit_call( struct emitter** mce, struct machine_ops* mop, struct frame* f, loperand closure, int nr_params, int nr_results );
void emit_ret( struct emitter** mce, struct machine_ops* mop, struct frame* f, loperand base, int nr_results );

#endif 
