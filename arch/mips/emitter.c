/*
* (C) Iain Fraser - GPLv3 
*
* MIPS machine code emitter. Uses own encoding code because dynasm for MIPS
* doesn't currently support register replacement. When time permits Ill
* update dynasm to do so.
*
* 1:1 mapping of LuaVM opcode to function emitter.
* TODO: remove this file 
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>
#include "mc_emitter.h"
#include "list.h"
#include "arch/mips/regdef.h"
#include "arch/mips/opcodes.h"
#include "arch/mips/emitter.h"
#include "arch/mips/vstack.h"
#include "arch/mips/regmap.h"
#include "bit_manip.h"
#include "table.h"
#include "func.h"
#include "operand.h"
#include "stack.h"

#if 0
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

static void call_fn( struct arch_emitter* me, uintptr_t fn, size_t argsz ){
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
}

void do_assign( struct arch_emitter* me, operand d, operand s ){
	assert( d.tag == OT_REG || d.tag == OT_DIRECTADDR );	

	int reg = d.tag == OT_REG ? d.reg : TEMP_REG1; 
	loadreg( me, &s, reg, d.tag == OT_REG );

	if( d.tag == OT_DIRECTADDR )		// | sw reg, d.addr 
		ENCODE_OP( me, GEN_MIPS_OPCODE_2REG( MOP_SW, d.base, s.reg, d.offset ) );
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

static void bop( struct arch_emitter* me, loperand d, loperand s, loperand t, int op, int special ){
	assert( d.islocal );
	do_bop( me, loperand_to_operand( me, d ).value,  loperand_to_operand( me, s ).value, 
				loperand_to_operand( me, t ).value, op, special );
}

#endif
