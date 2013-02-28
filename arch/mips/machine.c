/*
* (C) Iain Fraser - GPLv3  
* MIPs machine implementation. Uses the 32-bit generic emitter
* for machine code output. 
*/

#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include "machine.h"
#include "emitter32.h"
#include "arch/mips/regdef.h"
#include "arch/mips/opcodes.h"

// use the sp variable in machine struct 
#undef sp
#undef fp

struct machine_ops mips_ops;

#define _MOP	( &mips_ops )




static void load_bigim( struct emitter* me, int reg, int k ){
	ENCODE_OP( me, GEN_MIPS_OPCODE_2REG( MOP_LUI, 0, reg, ( k >> 16 ) & 0xffff ) );
	ENCODE_OP( me, GEN_MIPS_OPCODE_2REG( MOP_ORI, reg, reg, k & 0xffff ) );
}

static void loadim( struct emitter* me, int reg, int k ){
	if( k >= -32768 && k <= 65535 ){
		if( k >= 0 )
			ENCODE_OP( me, GEN_MIPS_OPCODE_2REG( MOP_ORI, _zero, reg, k ) );
		else
			ENCODE_OP( me, GEN_MIPS_OPCODE_2REG( MOP_ADDIU, _zero, reg, k ) );
	} else {
		load_bigim( me, reg, k );
	}
}

static void loadreg( struct emitter* me, operand* d, int temp_reg, bool forcereg ){
	switch( d->tag ){
		case OT_IMMED:
			loadim( me, temp_reg, d->k );	// only ADDi special case not worth extra work considering no cost for reg move
			break;
		case OT_DIRECTADDR:
			ENCODE_OP( me, GEN_MIPS_OPCODE_2REG( MOP_LW, d->base, temp_reg, d->offset ) );
			break;
		case OT_REG:
			if( !forcereg )
				return;
			ENCODE_OP( me, GEN_MIPS_OPCODE_3REG( MOP_SPECIAL, d->reg, _zero, temp_reg, MOP_SPECIAL_OR ) );
			break;
		default:
			assert( false );
	}

	// update to reg operand
	d->tag = OT_REG;
	d->reg = temp_reg;
} 


static void move( struct emitter* me, struct machine* m, operand d, operand s ){
	assert( d.tag == OT_REG || d.tag == OT_DIRECTADDR );	

	int reg = d.tag == OT_REG ? d.reg : acquire_temp( _MOP, me, m ); 
	loadreg( me, &s, reg, d.tag == OT_REG );

	if( d.tag == OT_DIRECTADDR ){		// | sw reg, d.addr 
		ENCODE_OP( me, GEN_MIPS_OPCODE_2REG( MOP_SW, d.base, s.reg, d.offset ) );
		release_temp( _MOP, me, m );
	}
}

/*
* Binary arithmetic operators
*/


static void do_bop( struct emitter* me, struct machine* m, operand d, operand s, operand t, int op, int special ){
	assert( d.tag == OT_REG || d.tag == OT_DIRECTADDR );

/*	int t0 = acquire_temp( _MOP, me, m );
	int t1 = acquire_temp( _MOP, me, m );

	loadreg( me, m, &s, t0, false );
	loadreg( me, m, &t, t1, false ); */

	int temps = load_coregisters( _MOP, me, m, 2, &s, &t );

	assert( s.tag == OT_REG && t.tag == OT_REG );

	int dreg = d.tag == OT_REG ? d.reg : s.reg;
	ENCODE_OP( me, GEN_MIPS_OPCODE_3REG( special, s.reg, t.reg, dreg, op ) );

	if( d.tag == OT_DIRECTADDR )		// | sw reg, d.addr 
		ENCODE_OP( me, GEN_MIPS_OPCODE_2REG( MOP_SW, d.base, dreg, d.offset ) );

	unload_coregisters( _MOP, me, m, temps );
}

static void do_div( struct emitter *me, struct machine* m, operand d, operand s, operand t, bool islow  ){
	operand nil = OP_TARGETREG( _zero );	// dst is in hi and lo see instruction encoding
	do_bop( me, m, nil, s, t, MOP_SPECIAL_DIV, MOP_SPECIAL );

	operand src = d;
	int temps = load_coregisters( _MOP, me, m, 1, &src );

	// load the quotient into the dest
	ENCODE_OP( me, GEN_MIPS_OPCODE_3REG( MOP_SPECIAL, _zero, _zero, src.reg, islow ? MOP_SPECIAL_MFLO : MOP_SPECIAL_MFHI ) );

	if( d.tag != OT_REG )
		move( me, m, d, src );

	unload_coregisters( _MOP, me, m , temps );	
}


static void add( struct emitter* me, struct machine* m, operand d, operand s, operand t ){
	do_bop( me, m, d, s, t, MOP_SPECIAL_ADDU, MOP_SPECIAL );
}

static void sub( struct emitter* me, struct machine* m, operand d, operand s, operand t ){
	do_bop( me, m, d, s, t, MOP_SPECIAL_SUBU, MOP_SPECIAL );
}

static void mul( struct emitter* me, struct machine* m, operand d, operand s, operand t ){
	do_bop( me, m, d, s, t, MOP_SPECIAL2_MUL, MOP_SPECIAL2 );
}

static void divide( struct emitter* me, struct machine* m, operand d, operand s, operand t ){
	do_div( me, m, d, s, t, true );
}

static void mod( struct emitter* me, struct machine* m, operand d, operand s, operand t ){
	do_div( me, m, d, s, t, false );
}

static void power( struct emitter* me, struct machine* m, operand d, operand s, operand t ){

}

/*
* Branching 
*/

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

void b( struct emitter* me, struct machine* m, label l ){
	EMIT( MI_B( branch( me, l ) ) );
	EMIT( MI_NOP() );
}


static void beq( struct emitter* me, struct machine* m, operand d, operand s, label l ){
	int temps = load_coregisters( _MOP, me, m, 2, &d, &s );

	EMIT( MI_BEQ( d.reg, s.reg, branch( me, l ) ) );
	EMIT( MI_NOP( ) );
	
	unload_coregisters( _MOP, me, m, temps );
}

static void blt( struct emitter* me, struct machine* m, operand d, operand s, label l ){
	;
}

static void bgt( struct emitter* me, struct machine* m, operand d, operand s, label l ){
	operand otemp = OP_TARGETREG( acquire_temp( _MOP, me, m ) );
	assert( otemp.tag == OT_REG );

	sub( me, m, otemp, d, s );
	
	EMIT( MI_BGTZ( otemp.reg, branch( me, l ) ) );
	EMIT( MI_NOP( ) );

	release_temp( _MOP, me, m );
}

static void ble( struct emitter* me, struct machine* m, operand d, operand s, label l ){
	;
}

static void bge( struct emitter* me, struct machine* m, operand d, operand s, label l ){
	operand otemp = OP_TARGETREG( acquire_temp( _MOP, me, m ) );
	assert( otemp.tag == OT_REG );

	sub( me, m, otemp, d, s );
	
	EMIT( MI_BGEZ( otemp.reg, branch( me, l ) ) );
	EMIT( MI_NOP( ) );

	release_temp( _MOP, me, m );
}


static void call( struct emitter* me, struct machine* m, operand fn ){
	int temps = load_coregisters( _MOP, me, m, 1, &fn );

	EMIT( MI_ADDIU( m->sp, m->sp, -4 ) );
	EMIT( MI_JALR( fn.reg ) );
	EMIT( MI_SW( _ra, m->sp, 0 ) );		// delay slot dummy
	EMIT( MI_ADDIU( m->sp, m->sp, 4 ) );

	unload_coregisters( _MOP, me, m, temps );
}

static void ret( struct emitter* me, struct machine* m ){
	EMIT( MI_JR( _ra ) );
	EMIT( MI_NOP( ) );
}


// TODO: take array of operands as argument
static void call_cfn( struct emitter* me, struct machine* m, uintptr_t fn, size_t argsz ){
#if 0
	// first 10 virtual regs mapped to temps so save them 
	int temps = min( 10, nr_livereg_vreg_occupy( me->nr_locals ) );
	for( int i=0; i < temps; i++ ){
		int treg = vreg_to_physical_reg( i );
		ENCODE_OP( me, GEN_MIPS_OPCODE_2REG( MOP_SW, _sp, treg, (int16_t)-( (i+1) * 4 ) ) );
	}
		
	// need space for temps and function arguments	
	int stackspace = 4 * ( max( argsz, 4 ) + temps );
	
	// create space for args - assume caller has setup a0 and a1
	ENCODE_OP( me, GEN_MIPS_OPCODE_2REG( MOP_ADDIU, _sp, _sp, (int16_t)( -stackspace ) ) );

	// can assume loading into _v0 is safe because function may overwrite it 
	loadim( me, _v0, fn );
	ENCODE_OP( me, GEN_MIPS_OPCODE_3REG( MOP_SPECIAL, _v0, _zero, _ra, MOP_SPECIAL_JALR ) );
	ENCODE_OP( me, MOP_NOP );	// TODO: one of the delay slots could be saved reg
	
	// restore the stack 	
	ENCODE_OP( me, GEN_MIPS_OPCODE_2REG( MOP_ADDIU, _sp, _sp, (int16_t)( stackspace ) ) );
	
	// reload temps
	for( int i=0; i < temps; i++ ){
		int treg = vreg_to_physical_reg( i );
		ENCODE_OP( me, GEN_MIPS_OPCODE_2REG( MOP_LW, _sp, treg, (int16_t)-( (i+1) * 4 ) ) );
	}
#endif 
}


struct machine_ops mips_ops = {
	.move = move,
	.add = add,
	.sub = sub,
	.mul = mul,
	.div = divide,
	.mod = mod,
	.pow = power,
	.b = b,
	.beq = beq,
	.blt = blt,
	.bgt = bgt,
	.ble = ble,
	.bge = bge,
	.call = call,
	.ret = ret,
	.call_cfn = call_cfn, 
	.create_emitter = emitter32_create 
};


struct machine mips_mach = {
	.sp = _sp,
	.fp = _fp,
	.nr_reg = 18 + 4,
	.nr_temp_regs = 4,
	.reg = {
		_a0,_a1,_a2,_a3,
		_s0,_s1,_s2,_s3,_s4,_s5,_s6,_s7,
		_t0,_t1,_t2,_t3,_t4,_t6,_t6,_t7,_t8,_t9
	}
};

