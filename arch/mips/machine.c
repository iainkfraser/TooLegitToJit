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
#include "arch/mips/arithmetic.h"
#include "arch/mips/branch.h"
#include "arch/mips/call.h"

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


void move( struct emitter* me, struct machine* m, operand d, operand s ){
	assert( d.tag == OT_REG || d.tag == OT_DIRECTADDR );	
	int reg;
	bool istemp = false;

	if( d.tag == OT_REG )
		reg = d.reg;
	else if( s.tag == OT_REG )
		reg = s.reg;
	else {
		reg = acquire_temp( _MOP, me, m );
		istemp = true;
	}

	loadreg( me, &s, reg, d.tag == OT_REG && s.tag == OT_REG );

	if( d.tag == OT_DIRECTADDR ){		// | sw reg, d.addr 
		EMIT( MI_SW( reg, d.base, d.offset ) );

		// NOT strong enough to veriy it was loaded in this function	
		if( istemp )	
			release_temp( _MOP, me, m );
	}
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
	.add = mips_add,
	.sub = mips_sub,
	.mul = mips_mul,
	.udiv = mips_udiv,
	.sdiv = mips_sdiv,
	.umod = mips_umod,
	.smod = mips_smod,
	.pow = mips_pow,
	.b = mips_b,
	.beq = mips_beq,
	.blt = mips_blt,
	.bgt = mips_bgt,
	.ble = mips_ble,
	.bge = mips_bge,
	.call = mips_call,
	.ret = mips_ret,
	.call_cfn = call_cfn, 
	.create_emitter = emitter32_create 
};

#undef ra 

struct machine mips_mach = {
	.sp = _sp,
	.fp = _fp,
	.ra = _ra, 
	.is_ra = true,
	.nr_reg = 18 + 4,
	.nr_temp_regs = 4,
#ifdef _ELFDUMP_
	.elf_endian = EI_DATA_LSB,
	.elf_machine = EM_MIPS,	
#endif
	.reg = {
		_a0,_a1,_a2,_a3,
		_s0,_s1,_s2,_s3,_s4,_s5,_s6,_s7,
		_t0,_t1,_t2,_t3,_t4,_t6,_t6,_t7,_t8,_t9
	}
};

