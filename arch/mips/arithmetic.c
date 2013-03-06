/*
* (C) Iain Fraser - GPLv3  
* MIPS arithmetic instructions implementation. 
*/

#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include "machine.h"
#include "macros.h"
#include "emitter32.h"
#include "arch/mips/regdef.h"
#include "arch/mips/opcodes.h"

extern void move( struct emitter* me, struct machine* m, operand d, operand s );

struct machine_ops mips_ops;

#define _MOP	( &mips_ops )
#define VALIDATE_OPERANDS( operator )					\
	do{									\
		assert( d.tag != OT_IMMED );					\
		if( s.tag == OT_IMMED &&  t.tag == OT_IMMED ){			\
			move( me, m, d, OP_TARGETIMMED( s.k operator t.k ) );	\
			return;							\
		}							\
	}while( 0 )

// can the operand be used in addi instruction?
static bool is_add_immed( operand o ){
	return o.tag == OT_IMMED && ( o.k >= -32768 && o.k <= 32767 );
}

struct aq_reg {
	operand d;	// original destination 
	int 	n;	// number of temps
};

static struct aq_reg acquire_reg( struct emitter* e, struct machine* m, operand* d, operand* s, operand* t, bool allowimmed ){
	assert( d->tag != OT_IMMED && s->tag != OT_IMMED );

	struct aq_reg a = { .d = *d, .n = 0 };
	
	if( d->tag != OT_REG ){
		*d = OP_TARGETREG( acquire_temp( _MOP, e, m ) );
		a.n++;
	}

	if( s->tag == OT_DIRECTADDR ){
		move( e, m, *d, *s );
		*s = *d;
	}
	
	if( !( t->tag == OT_REG || ( is_add_immed( *t ) && allowimmed ) ) ){
		operand newt;

		if( s == d ){
			a.n++;
			newt = OP_TARGETREG( acquire_temp( _MOP, e, m ) );
		} else {
			newt = *d;
		}
		
		move( e, m, newt, *t );
		*t = newt;
	}

	return a;
}

static void release_reg( struct emitter* e, struct machine* m, operand d, struct aq_reg a ){
	if( a.d.tag == OT_DIRECTADDR ){
		assert( d.tag == OT_REG );
		move( e, m, a.d, d );
	}

	release_tempn( _MOP, e, m, a.n );
}


static void do_add( struct emitter* me, struct machine* m, operand d, operand s, operand t, bool negate ){
	VALIDATE_OPERANDS( + );
	assert( !( s.tag == OT_IMMED && t.tag == OT_IMMED ) && d.tag != OT_IMMED );
	
	if( is_add_immed( s ) )
		swap( s, t );

	struct aq_reg a = acquire_reg( me, m, &d, &s, &t, true );
	assert( d.tag == OT_REG && s.tag == OT_REG );

	if( t.tag == OT_IMMED )
		EMIT( MI_ADDIU( d.reg, s.reg, t.k ) );
	else
		EMIT( MI_ADDU( d.reg, s.reg, t.reg ) );

	if( negate )
		EMIT( MI_NEGU( d.reg, d.reg ) );	
	
	release_reg( me, m, d, a );

}

void mips_add( struct emitter* me, struct machine* m, operand d, operand s, operand t ){
	return do_add( me, m, d, s, t, false );
}

extern void sub( struct emitter* me, struct machine* m, operand d, operand s, operand t );
void mips_sub( struct emitter* me, struct machine* m, operand d, operand s, operand t ){
	VALIDATE_OPERANDS( - );
	assert( !( s.tag == OT_IMMED && t.tag == OT_IMMED ) && d.tag != OT_IMMED );

	/* deal with immediate */
	if( is_add_immed( s ) ){
		s.k = -s.k;
		do_add( me, m, d, t, s, true ); 
	} else if( is_add_immed( t ) ) {
		t.k = -t.k;
		do_add( me, m, d, s, t, false );
	} else {
		struct aq_reg a = acquire_reg( me, m, &d, &s, &t, true );
		assert( d.tag == OT_REG && s.tag == OT_REG && t.tag == OT_REG );
		EMIT( MI_SUBU( d.reg, s.reg, t.reg ) );	
		release_reg( me, m, d, a ); 
//		sub( me, m, d, s, t );	
	}

}
