/*
* (C) Iain Fraser - GPLv3 
*
* Platform independent function/frame manipulation code.
*/

#ifndef _FRAME_H_
#define _FRAME_H_

#include "machine.h"
#include "emitter.h"

typedef struct frame {
	struct machine*		m;	
	int 			nr_locals;
	int			nr_params;
	int 			nr_consts;
	int			nr_temp_regs;  // first NR_TEMP_REGS registers are temps the rest are used for locals and consts

	// frame emission counters
	size_t			epi;	// epilogue location
	size_t			pro;	// prologue location

	operand*		consts;		
} frame;

// TODO: create macro for variable length generation ( flexible array member ) and dynamic 
//#define 


// set constants
void init_consts( struct frame* f, int n );
void setk_number( struct frame* f, int k, int value );

// function emission ( includes prologue, epilogue, constant loading and data section )
void emit_header( struct emitter* ae, struct frame* f );	// only call after all consts have been set
void emit_footer( struct emitter* ae, struct frame* f );	

// mapping
vreg_operand vreg_to_operand( struct frame* f, int vreg, bool stackonly );
vreg_operand const_to_operand( struct frame* f, int k );


#endif
