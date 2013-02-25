/*
* (C) Iain Fraser - GPLv3  
* MIPs machine implementation. Uses the 32-bit generic emitter
* for machine code output. 
*/

#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include "machine.h"
#include "emitter32.h"
#include "arch/mips/regdef.h"
#include "arch/mips/opcodes.h"
#include "arch/mips/regmap.h"

static void load_bigim( struct emitter* me, struct machine* m, int reg, int k ){
	ENCODE_OP( me, GEN_MIPS_OPCODE_2REG( MOP_LUI, 0, reg, ( k >> 16 ) & 0xffff ) );
	ENCODE_OP( me, GEN_MIPS_OPCODE_2REG( MOP_ORI, reg, reg, k & 0xffff ) );
}

static void loadim( struct emitter* me, struct machine* m, int reg, int k ){
	if( k >= -32768 && k <= 65535 ){
		if( k < 0 )
			ENCODE_OP( me, GEN_MIPS_OPCODE_2REG( MOP_ORI, _zero, reg, k ) );
		else
			ENCODE_OP( me, GEN_MIPS_OPCODE_2REG( MOP_ADDIU, _zero, reg, k ) );
	} else {
		load_bigim( me, m, reg, k );
	}
}

static void loadreg( struct emitter* me, struct machine* m, operand* d, int temp_reg, bool forcereg ){
	switch( d->tag ){
		case OT_IMMED:
			loadim( me, m, temp_reg, d->k );	// only ADDi special case not worth extra work considering no cost for reg move
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

	int reg = d.tag == OT_REG ? d.reg : TEMP_REG1; 
	loadreg( me, m, &s, reg, d.tag == OT_REG );

	if( d.tag == OT_DIRECTADDR )		// | sw reg, d.addr 
		ENCODE_OP( me, GEN_MIPS_OPCODE_2REG( MOP_SW, d.base, s.reg, d.offset ) );
}


static void do_bop( struct emitter* me, struct machine* m, operand d, operand s, operand t, int op, int special ){
	assert( d.tag == OT_REG || d.tag == OT_DIRECTADDR );

	loadreg( me, m, &s, TEMP_REG1, false );
	loadreg( me, m, &t, TEMP_REG2, false );

	assert( s.tag == OT_REG && t.tag == OT_REG );

	int dreg = d.tag == OT_REG ? d.reg : TEMP_REG0;
	ENCODE_OP( me, GEN_MIPS_OPCODE_3REG( special, s.reg, t.reg, dreg, op ) );

	if( d.tag == OT_DIRECTADDR )		// | sw reg, d.addr 
		ENCODE_OP( me, GEN_MIPS_OPCODE_2REG( MOP_SW, d.base, dreg, d.offset ) );
		 
}

static void do_div( struct emitter *me, struct machine* m, operand d, operand s, operand t, bool islow  ){
	operand nil = OP_TARGETREG( _zero );	// dst is in hi and lo see instruction encoding
	do_bop( me, m, nil, s, t, MOP_SPECIAL_DIV, MOP_SPECIAL );

	// load the quotient into the dest
	operand src = OP_TARGETREG( d.tag == OT_REG ? d.reg : TEMP_REG1 );
	ENCODE_OP( me, GEN_MIPS_OPCODE_3REG( MOP_SPECIAL, _zero, _zero, src.reg, islow ? MOP_SPECIAL_MFLO : MOP_SPECIAL_MFHI ) );

	if( d.tag != OT_REG )
		move( me, m, d, src );
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
	.call_cfn = call_cfn, 
	.create_emitter = emitter32_create 
};

