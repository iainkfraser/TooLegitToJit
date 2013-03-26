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
#include "arch/mips/machine.h"
#include "arch/mips/regdef.h"
#include "arch/mips/opcodes.h"
#include "arch/mips/arithmetic.h"
#include "arch/mips/branch.h"
#include "arch/mips/call.h"
#include "arch/mips/jfunc.h"

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

bool is_mips_temp( operand o ){
	return ISO_REG( o ) && MIPSREG_ISTEMP( o.reg );
}

operand mips_carg( struct machine* m, int argidx ){
	return argidx < 4 ?
		OP_TARGETREG( _a0 + argidx ) :
		OP_TARGETDADDR( m->sp, 16 + 4 * argidx );

}

operand mips_cret( struct machine* m ){
	return OP_TARGETREG( _v0 );	
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
	.bor = mips_or,
	.b = mips_b,
	.beq = mips_beq,
	.blt = mips_blt,
	.bgt = mips_bgt,
	.ble = mips_ble,
	.bge = mips_bge,
	.call = mips_call,
	.ret = mips_ret,
	.call_static_cfn = mips_static_ccall
	,.create_emitter = emitter32_create 
	,.nr_jfuncs = mips_nr_jfuncs
	,.jf_init = mips_jf_init
	,.carg = mips_carg
	,.cret = mips_cret
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

