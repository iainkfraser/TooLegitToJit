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


// this function could be inline
void syn_memcpyw( struct machine_ops* mop, struct emitter* e, struct machine* m, operand d, operand s, operand size ){
	operand iter = OP_TARGETREG( acquire_temp( mop, e, m ) );			
	operand src = OP_TARGETREG( acquire_temp( mop, e, m ) );
	operand dst = OP_TARGETREG( acquire_temp( mop, e, m ) );

	// init iterator
	mop->move( e, m, iter, OP_TARGETIMMED( 0 ) );
	mop->move( e, m, src, s );
	mop->move( e, m, dst, d );
	
	// start loop 
	e->ops->label_local( e, 0 );
	mop->beq( e, m, iter, size, LBL_NEXT( 0 ) );  

	// copy 
	mop->move( e, m, OP_TARGETDADDR( dst.reg, 0 ), OP_TARGETDADDR( src.reg, 0 ) );
	
	// update pointers 
	mop->add( e, m, dst, dst, OP_TARGETIMMED( 4 ) );
	mop->add( e, m, src, src, OP_TARGETIMMED( 4 ) ); 
	
	// update iterator
	mop->add( e, m, iter, iter, OP_TARGETIMMED( 1 ) );
	
	mop->b( e, m, LBL_PREV( 0 ) );
	e->ops->label_local( e, 0 );
	
	release_tempn( mop, e, m, 3 );
}

void syn_memsetw( struct machine_ops* mop, struct emitter* e, struct machine* m, operand d, operand v, operand size ){
	operand iter = OP_TARGETREG( acquire_temp( mop, e, m ) );			
	operand dst = OP_TARGETREG( acquire_temp( mop, e, m ) );

	// init iterator
	mop->move( e, m, iter, OP_TARGETIMMED( 0 ) );
	mop->move( e, m, dst, d );
	
	// start loop 
	e->ops->label_local( e, 0 );
	mop->beq( e, m, iter, size, LBL_NEXT( 0 ) );  

	// copy 
	mop->move( e, m, OP_TARGETDADDR( dst.reg, 0 ), v );
	
	// update pointers 
	mop->add( e, m, dst, dst, OP_TARGETIMMED( 4 ) );
	
	// update iterator
	mop->add( e, m, iter, iter, OP_TARGETIMMED( 1 ) );
	
	mop->b( e, m, LBL_PREV( 0 ) );
	e->ops->label_local( e, 0 );
	
	release_tempn( mop, e, m, 2 );
}

void syn_min( struct machine_ops* mop, struct emitter* e, struct machine* m, operand d, operand s, operand t ){
	mop->blt( e, m, s, t, LBL_NEXT( 0 ) );
	mop->move( e, m, d, t );
	mop->b( e, m, LBL_NEXT( 1 ) );
	e->ops->label_local( e, 0 );
	mop->move( e, m, d, s );
	e->ops->label_local( e, 1 );
}

void syn_max( struct machine_ops* mop, struct emitter* e, struct machine* m, operand d, operand s, operand t ){
	mop->blt( e, m, s, t, LBL_NEXT( 0 ) );
	mop->move( e, m, d, t );
	mop->b( e, m, LBL_NEXT( 1 ) );
	e->ops->label_local( e, 0 );
	mop->move( e, m, d, s );
	e->ops->label_local( e, 1 );
}

