/*
* (C) Iain Fraser - GPLv3  
* MIPS branch instructions implementation. 
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

extern void move( struct emitter* me, struct machine* m, operand d, operand s );

struct machine_ops mips_ops;
#define _MOP	( &mips_ops )
#define VALIDATE_OPERANDS( operator )					\
	do{									\
		assert( s.tag != OT_IMMED );					\
		if( s.tag == OT_IMMED &&  t.tag == OT_IMMED ){			\
			if( s.k operator t.k ){					\
				EMIT( MI_B( branch( me, l ) ) );		\
				EMIT( MI_NOP() );				\
			}							\
			return;							\
		}							\
	}while( 0 )

static int branch( struct emitter* me, label l ){
	switch( l.tag ){
		case LABEL_LOCAL:
			me->ops->branch_local( me, l.local, l.isnext );
			break;
		case LABEL_PC:
			me->ops->branch_pc( me, l.vline );
			break;
		case LABEL_EC:
			return l.ec - ( me->ops->ec( me ) + 1 ); 		
	}

	return 0;
}

void mips_b( struct emitter* me, struct machine* m, label l ){
	EMIT( MI_B( branch( me, l ) ) );
	EMIT( MI_NOP() );
}


void mips_beq( struct emitter* me, struct machine* m, operand s, operand t, label l ){
	VALIDATE_OPERANDS( == );

	int temps = load_coregisters( _MOP, me, m, 2, &s, &t );

	EMIT( MI_BEQ( s.reg, t.reg, branch( me, l ) ) );
	EMIT( MI_NOP( ) );
	
	unload_coregisters( _MOP, me, m, temps );
}

static int ineq_load_temp( operand* op, struct emitter* e, struct machine* m ){
	if( op->tag != OT_DIRECTADDR )
		return 0;

	*op = OP_TARGETREG( acquire_temp( _MOP, e, m ) );
	return 1;	
}

static void mips_inequality( struct emitter* me, struct machine* m, operand s, operand t, label l, 
		int op, int subop, int antiop, int antisubop ){
	assert( !( s.tag == OT_IMMED && t.tag == OT_IMMED ) ); 
	if( s.tag == OT_IMMED ){
		mips_inequality( me, m, t, s, l, antiop, antisubop, op, subop ); 
		return;
	}

	// TODO: better encoding scheme? 
	int temps = 0;
	temps |= ineq_load_temp( &s, me, m ) << 0;
	temps |= ineq_load_temp( &t, me, m ) << 1;

	operand d;
	if( temps != 0 )
		d = temps & 0x1 ? s : t;
	else
		d = s;

	mips_sub( me, m, d, s, t );

	if( temps == 3 )
		release_temp( _MOP, me, m ); 

	GEN_MIPS_OPCODE_2REG( op, d.reg, subop, branch( me, l ) );

	if( temps != 0 ){
		if( !release_temp( _MOP, me, m ) )
			EMIT( MI_NOP() );
	} else	{
		mips_add( me, m, d, d, t );	
	}
} 


void mips_blt( struct emitter* me, struct machine* m, operand s, operand t, label l ){
	VALIDATE_OPERANDS( < );
	mips_inequality( me, m, s, t, l, MOP_BLTZ, 0, MOP_BGTZ, 0 );
}

void mips_bgt( struct emitter* me, struct machine* m, operand s, operand t, label l ){
	VALIDATE_OPERANDS( > );
	mips_inequality( me, m, s, t, l, MOP_BGTZ, 0, MOP_BLTZ, 0 );
}

void mips_ble( struct emitter* me, struct machine* m, operand s, operand t, label l ){
	VALIDATE_OPERANDS( <= );
	mips_inequality( me, m, s, t, l, MOP_BLEZ, 0, MOP_BGEZ, 1 );
}

void mips_bge( struct emitter* me, struct machine* m, operand s, operand t, label l ){
	VALIDATE_OPERANDS( >= );
	mips_inequality( me, m, s, t, l, MOP_BGEZ, 1, MOP_BLEZ, 0 );
}



