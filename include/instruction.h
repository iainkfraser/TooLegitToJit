/*
* (C) Iain Fraser - GPLv3 
*
* 1:1 mapping of LuaVM instructions
*/

#ifndef _INSTRUCTION_H_
#define _INSTRUCTION_H_

#include "mc_emitter.h"
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


void emit_loadk( struct arch_emitter** mce, struct frame* f, int l, int k );
void emit_move( struct arch_emitter** mce, struct frame* f, loperand d, loperand s );

void emit_add( struct arch_emitter** mce, struct frame* f, loperand d, loperand s, loperand t );
void emit_sub( struct arch_emitter** mce, struct frame* f, loperand d, loperand s, loperand t );
void emit_mul( struct arch_emitter** mce, struct frame* f, loperand d, loperand s, loperand t );
void emit_div( struct arch_emitter** mce, struct frame* f, loperand d, loperand s, loperand t );
void emit_mod( struct arch_emitter** mce, struct frame* f, loperand d, loperand s, loperand t );

void emit_forprep( struct arch_emitter** mce, struct frame* f, loperand init, int pc, int j );
void emit_forloop( struct arch_emitter** mce, struct frame* f, loperand loopvar, int pc, int j );

void emit_gettable( struct arch_emitter** mce, struct frame* f, loperand dst, loperand table, loperand idx );
void emit_newtable( struct arch_emitter** mce, struct frame* f, loperand dst, int array, int hash );
void emit_setlist( struct arch_emitter** mce, struct frame* f, loperand table, int n, int block );

void emit_closure( struct arch_emitter** mce, struct frame* f, loperand dst, struct proto* p );
void emit_call( struct arch_emitter** mce, struct frame* f, loperand closure, int nr_params, int nr_results );
void emit_ret( struct arch_emitter** mce, struct frame* f );

#endif 
