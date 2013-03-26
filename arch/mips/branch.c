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
#include "arch/mips/machine.h"
#include "arch/mips/regdef.h"
#include "arch/mips/opcodes.h"
#include "arch/mips/arithmetic.h"


#define VALIDATE_OPERANDS( operator )					\
	do{									\
		assert( s.tag != OT_IMMED );					\
		if( s.tag == OT_IMMED &&  t.tag == OT_IMMED ){			\
			if( s.k operator t.k ){					\
				mips_b( me, m, l );				\
			}							\
			return;							\
		}							\
		sub_zero_reg( &s );					\
		sub_zero_reg( &t );					\
	}while( 0 )

int mips_branch( struct emitter* me, label l ){
	switch( l.tag ){
		case LABEL_LOCAL:
			me->ops->branch_local( me, l.local, l.isnext );
			break;
		case LABEL_PC:
			me->ops->branch_pc( me, l.vline );
			break;
		case LABEL_EC:
			return l.ec - ( me->ops->ec( me ) + 1 ); 	
		case LABEL_ABSOLUTE:
			assert( ISO_IMMED( l.abs ) );	// branch ops don't do indirect for now
			assert( false );// TODO: return difference from here to there
			break;
		default:
			assert( false );	
	}

	return 0;
}


#define IS_INVERTIBLE_IMMED( t )	( is_add_immed( t ) && is_sub_immed( t ) )
#define RELEASE_OR_NOP( me, m )				\
	do{						\
		if( !release_temp( _MOP, me, m ) )	\
			EMIT( MI_NOP() );		\
	}while( 0 )

void mips_b( struct emitter* me, struct machine* m, label l ){
	
	if( !ISL_ABS( l ) ){
		EMIT( MI_B( mips_branch( me, l ) ) );
	} else if ( ISL_ABSDIRECT( l ) ){	// TODO: make sure within 256MB region by calling MI_J_SAFE
		EMIT( MI_J( l.abs.k ) );
	} else if ( ISO_REG( l.abs ) ) {
		EMIT( MI_JR( l.abs.reg ) );
	} else { 
		assert( ISO_DADDR( l.abs ) );
		operand t = OP_TARGETREG( acquire_temp( _MOP, me, m ) );
		move( me, m, t, l.abs );
		EMIT( MI_JR( t.reg ) );	
		RELEASE_OR_NOP( me, m );
		return;
	}

	// delay slot
	EMIT( MI_NOP() );
}

void mips_beq( struct emitter* me, struct machine* m, operand s, operand t, label l ){
	VALIDATE_OPERANDS( == );
	bool tried = false;
rematch: 
	/* try pattern matching approach to instruction selection */
	if( ISO_REG( s ) && ISO_REG( t ) ){
		EMIT( MI_BEQ( s.reg, t.reg, mips_branch( me, l ) ) );
		EMIT( MI_NOP( ) );
	} else if ( ISO_REG( s ) && ISO_DADDR( t ) ){
		operand temp = OP_TARGETREG( acquire_temp( _MOP, me, m ) );
		move( me, m, temp, t ); 
		EMIT( MI_BEQ( s.reg, temp.reg, mips_branch( me, l ) ) );
		RELEASE_OR_NOP( me, m );	
	} else if ( ISO_REG( s ) && IS_INVERTIBLE_IMMED( t ) ){
		EMIT( MI_ADDIU( s.reg, s.reg, -t.k ) );
		EMIT( MI_BEQ( s.reg, _zero, mips_branch( me, l ) ) );
		EMIT( MI_ADDIU( s.reg, s.reg, t.k ) );
	} else if ( ISO_DADDR( s ) && ISO_DADDR( t ) ){
		operand t1 = OP_TARGETREG( acquire_temp( _MOP, me, m ) );
		operand t2 = OP_TARGETREG( acquire_temp( _MOP, me, m ) );
		move( me, m, t1, s ); 
		move( me, m, t2, t ); 
		EMIT( MI_SUBU( t1.reg, t1.reg, t2.reg ) );
		release_temp( _MOP, me, m );
		EMIT( MI_BEQ( t1.reg, _zero, mips_branch( me, l ) ) );
		RELEASE_OR_NOP( me, m );	
	} else if ( ISO_DADDR( s ) && IS_INVERTIBLE_IMMED( t ) ){
		operand temp = OP_TARGETREG( acquire_temp( _MOP, me, m ) );
		move( me, m, temp, s ); 
		EMIT( MI_ADDIU( temp.reg, temp.reg, -t.k ) );
		EMIT( MI_BEQ( temp.reg, _zero, mips_branch( me, l ) ) );
		RELEASE_OR_NOP( me, m );
	} else {
		assert( !tried );
		tried = true;
		swap( s, t );
		goto rematch;
	}
}

static int ineq_load_temp( operand* op, struct emitter* e, struct machine* m ){
	if( ISO_REG( *op ) || IS_INVERTIBLE_IMMED( *op )  )
		return false;

	operand t = OP_TARGETREG( acquire_temp( _MOP, e, m ) );
	move( e, m, t, *op );
	*op = t;
	return true;	
}

static void mips_inequality( struct emitter* me, struct machine* m, operand s, operand t, label l, 
		int op, int subop, int antiop, int antisubop ){
	assert( !( s.tag == OT_IMMED && t.tag == OT_IMMED ) ); 
	bool iss, ist;
	operand d;
	
	if( s.tag == OT_IMMED ){
		mips_inequality( me, m, t, s, l, antiop, antisubop, op, subop ); 
		return;
	}

	iss = ineq_load_temp( &s, me, m );
	ist = ineq_load_temp( &t, me, m );

	// set branch compare reg ( prefer temps )
	if( iss )
		d = s;
	else if( ist )
		d = t;
	else 
		d = s;	

	mips_sub( me, m, d, s, t );

	if( iss && ist )
		release_temp( _MOP, me, m ); 

	ENCODE_OP( me, GEN_MIPS_OPCODE_2REG( op, d.reg, subop, mips_branch( me, l ) ) );

	if( iss || ist )
		RELEASE_OR_NOP( me, m );
	else
		mips_add( me, m, d, d, t );	
	
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



