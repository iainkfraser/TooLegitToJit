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
#include "bit_manip.h"
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
bool is_add_immed( operand o ){
	return o.tag == OT_IMMED && ( o.k >= -32768 && o.k <= 32767 );
}


bool is_sub_immed( operand o ){
	if( o.tag == OT_IMMED ){
		o.k = -o.k;
		return is_add_immed( o );
	}
	
	return false;
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

	if( !( s->tag == OT_REG || ( is_add_immed( *s ) && allowimmed ) )  ){
		operand news;

		if( t->tag == OT_REG && t->reg == d->reg )
			news = OP_TARGETREG( acquire_temp( _MOP, e, m ) ), a.n++;				
		else
			news = *d;
		
		move( e, m, news, *s );
		*s = news;
	}
	
	if( !( t->tag == OT_REG || ( is_add_immed( *t ) && allowimmed ) ) ){
		operand newt;

		if( s->tag == OT_REG && s->reg == d->reg )
			newt = OP_TARGETREG( acquire_temp( _MOP, e, m ) ), a.n++;
		else
			newt = *d;
		
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

static void do_nonimm_bop( struct emitter* me, struct machine* m, operand d, operand s, operand t, int op, int special,
		bool isdiv, bool islow ){
	struct aq_reg a = acquire_reg( me, m, &d, &s, &t, false );
	assert( d.tag == OT_REG && s.tag == OT_REG && t.tag == OT_REG );

	ENCODE_OP( me, GEN_MIPS_OPCODE_3REG( special, s.reg, t.reg, isdiv ? _zero : d.reg, op ) );

	if( isdiv )
		ENCODE_OP( me,	GEN_MIPS_OPCODE_3REG( MOP_SPECIAL, _zero, _zero, d.reg, 
					islow ? MOP_SPECIAL_MFLO : MOP_SPECIAL_MFHI ) );
	release_reg( me, m, d, a ); 
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

void mips_sub( struct emitter* me, struct machine* m, operand d, operand s, operand t ){
	VALIDATE_OPERANDS( - );
	assert( !( s.tag == OT_IMMED && t.tag == OT_IMMED ) && d.tag != OT_IMMED );

	/* deal with immediate */
	if( is_sub_immed( s ) ){	
		s.k = -s.k;
		do_add( me, m, d, t, s, true ); 
	} else if( is_sub_immed( t ) ) {
		t.k = -t.k;
		do_add( me, m, d, s, t, false );
	} else {
		do_nonimm_bop( me, m, d, s, t, MOP_SPECIAL_SUBU, MOP_SPECIAL, false, false );
	}

}

void mips_mul( struct emitter* me, struct machine* m, operand d, operand s, operand t ){
	VALIDATE_OPERANDS( * );
	if( is_add_immed( s ) && ispowerof2( s.k ) )
		swap( s, t );
	
	if( is_add_immed( t ) && ispowerof2( t.k ) ){
		const int sk = ilog2( t.k );	
		struct aq_reg a = acquire_reg( me, m, &d, &s, &t, true );
		assert( d.tag == OT_REG && s.tag == OT_REG && t.tag == OT_IMMED );		
		EMIT( MI_SLL( d.reg, s.reg, sk ) );
		release_reg( me, m, d, a ); 
	} else {
		do_nonimm_bop( me, m, d, s, t, MOP_SPECIAL2_MUL, MOP_SPECIAL2, false, false );
	}
}

static void mips_div( struct emitter* me, struct machine* m, operand d, operand s, operand t, bool issigned ){
	VALIDATE_OPERANDS( / );
	if( is_add_immed( t ) && ispowerof2( t.k ) ){
		/* IS IT SIGNED OR UNSIGNED? */
		const int sk = ilog2( t.k );	
		struct aq_reg a = acquire_reg( me, m, &d, &s, &t, true );
		assert( d.tag == OT_REG && s.tag == OT_REG && t.tag == OT_IMMED );		

		if( issigned )
			EMIT( MI_SRA( d.reg, s.reg, sk ) );
		else
			EMIT( MI_SRL( d.reg, s.reg, sk ) );

		release_reg( me, m, d, a ); 
	} else {
		if( issigned )
			do_nonimm_bop( me, m, d, s, t, MOP_SPECIAL_DIV, MOP_SPECIAL, true, true );
		else
			do_nonimm_bop( me, m, d, s, t, MOP_SPECIAL_DIVU, MOP_SPECIAL, true, true );
	}
}

void mips_sdiv( struct emitter* me, struct machine* m, operand d, operand s, operand t ){
	mips_div( me, m, d, s, t, true );
}

void mips_udiv( struct emitter* me, struct machine* m, operand d, operand s, operand t ){
	mips_div( me, m, d, s, t, false );
}

void mips_smod( struct emitter* me, struct machine* m, operand d, operand s, operand t ){
	VALIDATE_OPERANDS( % );
	do_nonimm_bop( me, m, d, s, t, MOP_SPECIAL_DIV, MOP_SPECIAL, true, false );
}

void mips_umod( struct emitter* me, struct machine* m, operand d, operand s, operand t ){
	do_nonimm_bop( me, m, d, s, t, MOP_SPECIAL_DIVU, MOP_SPECIAL, true, false );
}

void mips_pow( struct emitter* me, struct machine* m, operand d, operand s, operand t ){
	// TODO: 
}
