/*
* (C) Iain Fraser - GPLv3  
* Sythentic generic machine instructions.
*/

#include <stdarg.h>
#include "emitter.h"
#include "machine.h"

void assign( struct machine_ops* mop, struct emitter* e, struct machine* m, vreg_operand d, vreg_operand s ) {
	mop->move( e, m, d.value, s.value );
	mop->move( e, m, d.type, s.type );
}


void loadim( struct machine_ops* mop, struct emitter* e, struct machine* m, int reg, int value ) {
	operand r = OP_TARGETREG( reg );
	operand v = OP_TARGETIMMED( value );
	mop->move( e, m, r, v );
}

void pushn( struct machine_ops* mop, struct emitter* e, struct machine* m, int nr_operands, ... ){
	va_list ap;
	const operand stack = OP_TARGETREG( m->sp );

	va_start( ap, nr_operands );

	if( nr_operands == 1 && mop->push ){
		mop->push( e, m, va_arg( ap, operand ) );
	} else {
		for( int i = 0; i < nr_operands; i++ )
			mop->move( e, m, OP_TARGETDADDR( m->sp, -4 * ( i + 1 )	), va_arg( ap, operand ) );
		
		mop->add( e, m, stack, stack, OP_TARGETIMMED( -4 * nr_operands ) ); 
	}

	va_end( ap );
}

void popn( struct machine_ops* mop, struct emitter* e, struct machine* m, int nr_operands, ... ){
	va_list ap;
	const operand stack = OP_TARGETREG( m->sp );

	va_start( ap, nr_operands );

	if( nr_operands == 1 && mop->pop ){
		mop->pop( e, m, va_arg( ap, operand ) );
	} else {
		for( int i = 0; i < nr_operands; i++ )
			mop->move( e, m, va_arg( ap, operand ), OP_TARGETDADDR( m->sp, 4 * i ) );
		
		mop->add( e, m, stack, stack, OP_TARGETIMMED( 4 * nr_operands ) ); 
	}

	va_end( ap );
}
