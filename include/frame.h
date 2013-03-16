/*
* (C) Iain Fraser - GPLv3 
*
* Platform independent function/frame manipulation code.
*/

#ifndef _FRAME_H_
#define _FRAME_H_

#include "machine.h"
#include "emitter.h"
#include "lobject.h"

typedef struct frame {
	struct machine*		m;	
	int 			nr_locals;
	int			nr_params;
	int 			nr_consts;

	// frame emission counters
	size_t			epi;	// epilogue location
	size_t			pro;	// prologue location

	vreg_operand*		consts;		
} frame;

// TODO: create macro for variable length generation ( flexible array member ) and dynamic 
//#define 


int code_start( struct frame* f );

// set constants
void init_consts( struct frame* f, int n );
void setk_number( struct frame* f, int k, int value );
void setk_string( struct frame* f, int k, char* str );

// function emission ( includes prologue, epilogue, constant loading and data section )
void emit_header( struct machine_ops* mop, struct emitter* e, struct frame* f );	// only call after all consts have been set
void emit_footer( struct machine_ops* mop, struct emitter* e, struct frame* f );	

// mapping
vreg_operand vreg_to_operand( struct frame* f, int vreg, bool stackonly );
vreg_operand const_to_operand( struct frame* f, int k );

// pointer mapping for stack only
intptr_t vreg_value_offset( int idx );
intptr_t vreg_type_offset( int idx ); 
struct TValue* vreg_tvalue_offset( struct TValue* base, int idx );

operand get_frame_closure( struct frame* f );

#endif
