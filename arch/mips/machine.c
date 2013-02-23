/*
* (C) Iain Fraser - GPLv3  
* MIPs machine implementation. 
* TODO: make it depedent on generic machine emitter 
*/

#include <stdlib.h>
#include <assert.h>
#include "mc_emitter.h"
#include "machine.h"
#include "arch/mips/regdef.h"
#include "arch/mips/opcodes.h"
#include "arch/mips/emitter.h"
#include "arch/mips/regmap.h"

void load_bigim( struct arch_emitter* me, int reg, int k ){
	ENCODE_OP( me, GEN_MIPS_OPCODE_2REG( MOP_LUI, 0, reg, ( k >> 16 ) & 0xffff ) );
	ENCODE_OP( me, GEN_MIPS_OPCODE_2REG( MOP_ORI, reg, reg, k & 0xffff ) );
}

void loadim( struct arch_emitter* me, int reg, int k ){
	if( k >= -32768 && k <= 65535 ){
		if( k < 0 )
			ENCODE_OP( me, GEN_MIPS_OPCODE_2REG( MOP_ORI, _zero, reg, k ) );
		else
			ENCODE_OP( me, GEN_MIPS_OPCODE_2REG( MOP_ADDIU, _zero, reg, k ) );
	} else {
		load_bigim( me, reg, k );
	}
}

static void loadreg( struct arch_emitter* me, operand* d, int temp_reg, bool forcereg ){
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

static void do_bop( struct arch_emitter* me, operand d, operand s, operand t, int op, int special ){
	assert( d.tag == OT_REG || d.tag == OT_DIRECTADDR );

	loadreg( me, &s, TEMP_REG1, false );
	loadreg( me, &t, TEMP_REG2, false );

	assert( s.tag == OT_REG && t.tag == OT_REG );

	int dreg = d.tag == OT_REG ? d.reg : TEMP_REG0;
	ENCODE_OP( me, GEN_MIPS_OPCODE_3REG( special, s.reg, t.reg, dreg, op ) );

	if( d.tag == OT_DIRECTADDR )		// | sw reg, d.addr 
		ENCODE_OP( me, GEN_MIPS_OPCODE_2REG( MOP_SW, d.base, dreg, d.offset ) );
		 
}

static void do_div( struct arch_emitter *me, operand d, operand s, operand t, bool islow  ){
	operand nil = OP_TARGETREG( _zero );	// dst is in hi and lo see instruction encoding
	do_bop( me, nil, s, t, MOP_SPECIAL_DIV, MOP_SPECIAL );

	// load the quotient into the dest
	operand src = OP_TARGETREG( d.tag == OT_REG ? d.reg : TEMP_REG1 );
	ENCODE_OP( me, GEN_MIPS_OPCODE_3REG( MOP_SPECIAL, _zero, _zero, src.reg, islow ? MOP_SPECIAL_MFLO : MOP_SPECIAL_MFHI ) );

	if( d.tag != OT_REG )
		arch_move( me, d, src );
}


void arch_move( struct arch_emitter* me, operand d, operand s ){
	assert( d.tag == OT_REG || d.tag == OT_DIRECTADDR );	

	int reg = d.tag == OT_REG ? d.reg : TEMP_REG1; 
	loadreg( me, &s, reg, d.tag == OT_REG );

	if( d.tag == OT_DIRECTADDR )		// | sw reg, d.addr 
		ENCODE_OP( me, GEN_MIPS_OPCODE_2REG( MOP_SW, d.base, s.reg, d.offset ) );
}


void arch_add( struct arch_emitter* me, operand d, operand s, operand t ){
	do_bop( me, d, s, t, MOP_SPECIAL_ADDU, MOP_SPECIAL );
}

void arch_sub( struct arch_emitter* me, operand d, operand s, operand t ){
	do_bop( me, d, s, t, MOP_SPECIAL_SUBU, MOP_SPECIAL );
}

void arch_mul( struct arch_emitter* me, operand d, operand s, operand t ){
	do_bop( me, d, s, t, MOP_SPECIAL2_MUL, MOP_SPECIAL2 );
}

void arch_div( struct arch_emitter* me, operand d, operand s, operand t ){
	do_div( me, d, s, t, true );
}

void arch_mod( struct arch_emitter* me, operand d, operand s, operand t ){
	do_div( me, d, s, t, false );
}

void arch_pow( struct arch_emitter* me, operand d, operand s, operand t ){

}

// TODO: take array of operands as argument
void arch_call_cfn( struct arch_emitter* me, uintptr_t fn, size_t argsz ){

}
