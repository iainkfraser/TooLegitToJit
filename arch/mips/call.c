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

// use the sp variable in machine struct 
#undef sp
#undef fp

extern struct machine_ops mips_ops;

#define _MOP	( &mips_ops )

/*
* TODO: write up in machine spec that only b and call need to do absolute addressing.
*/
void mips_call( struct emitter* me, struct machine* m, label l ){
	
	if( ISL_ABS( l ) ){
		if( ISL_ABSDIRECT( l ) ){
			// TODO: Make sure within 256MB region 
			EMIT( MI_SW( _ra, m->sp, -4 ) );		// DO NOT PUT in delay slot because ra already overwritten.
			EMIT( MI_JAL( l.abs.k ) );
			EMIT( MI_ADDIU( m->sp, m->sp, -4 ) );	// delay slot
			EMIT( MI_LW( _ra, m->sp, 0 ) );
			EMIT( MI_ADDIU( m->sp, m->sp, 4 ) );
		} else { 
			operand fn = l.abs;
			int temps = load_coregisters( _MOP, me, m, 1, &fn );
			EMIT( MI_SW( _ra, m->sp, -4 ) );		// DO NOT PUT in delay slot because ra already overwritten.
			EMIT( MI_JALR( fn.reg ) );
			EMIT( MI_ADDIU( m->sp, m->sp, -4 ) );	// delay slot
			EMIT( MI_LW( _ra, m->sp, 0 ) );
			EMIT( MI_ADDIU( m->sp, m->sp, 4 ) );
			unload_coregisters( _MOP, me, m, temps );
		}
	} else {
		// TODO: JALR
	}
}

void mips_ret( struct emitter* me, struct machine* m ){
	EMIT( MI_JR( _ra ) );
	EMIT( MI_NOP( ) );
}
