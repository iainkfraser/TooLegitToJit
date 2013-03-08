/*
* (C) Iain Fraser - GPLv3  
* MIPS call instructions implementation. 
*/

#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include "machine.h"
#include "macros.h"
#include "emitter32.h"
#include "bit_manip.h"
#include "arch/mips/regdef.h"
#include "arch/mips/opcodes.h"
#include "arch/mips/arithmetic.h"
#include "arch/mips/call.h"
#include "arch/mips/branch.h"

// use the sp variable in machine struct 
#undef sp
#undef fp

extern struct machine_ops mips_ops;

#define _MOP	( &mips_ops )

/*
* TODO: write up in machine spec that only b and call need to do absolute addressing.
*/

static inline void call_prologue( struct emitter* me, struct machine* m ){
	EMIT( MI_SW( _ra, m->sp, -4 ) );		// DO NOT PUT in delay slot because ra already overwritten.
}

static inline void call_epilogue( struct emitter* me, struct machine* m ){
	EMIT( MI_ADDIU( m->sp, m->sp, -4 ) );	// delay slot
	EMIT( MI_LW( _ra, m->sp, 0 ) );
	EMIT( MI_ADDIU( m->sp, m->sp, 4 ) );
}

void mips_call( struct emitter* me, struct machine* m, label l ){
	operand fn;
	int temps;	
	
	if( ISL_ABS( l ) ){
		if( ISL_ABSDIRECT( l ) ){
			call_prologue( me, m );
			EMIT( MI_JAL( l.abs.k ) );	// TODO: make safe_JAL which takes current location to check for 256mb bounds
			call_epilogue( me, m );	
		} else { 
			fn = l.abs;
			temps = load_coregisters( _MOP, me, m, 1, &fn );
			call_prologue( me, m );
			EMIT( MI_JALR( fn.reg ) );
			call_epilogue( me, m );
			unload_coregisters( _MOP, me, m, temps );
		}
	} else {
		call_prologue( me, m );
		EMIT( MI_BAL( mips_branch( me, l ) ) );
		call_epilogue( me, m );
	}

}

void mips_ret( struct emitter* me, struct machine* m ){
	EMIT( MI_JR( _ra ) );
	EMIT( MI_NOP( ) );
}
