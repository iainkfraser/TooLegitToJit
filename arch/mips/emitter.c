/*
* (C) Iain Fraser - GPLv3 
*
* MIPS machine code emitter. Uses own encoding code because dynasm for MIPS
* doesn't currently support register replacement. When time permits Ill
* update dynasm to do so.
*
* 1:1 mapping of LuaVM opcode to function emitter.
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

#define REF	( *( arch_emitter**)mce ) 

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

// Load to register IF NOT IN REGISTER, different to ASSIGN 
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



void emit_ret( arch_emitter** mce ){
	int j = ( REF->epi - ( REF->size + 1 ) ) / 4;	// branch is from delay slot
	ENCODE_OP( REF, GEN_MIPS_OPCODE_2REG( MOP_BEQ, _zero, _zero, (int16_t)j ) );
	ENCODE_OP( REF, MOP_NOP );
} 


void emit_loadk( arch_emitter** mce, int l, int k ){
	do_assign( REF, llocal_to_operand( REF, l ).value, lconst_to_operand( REF, k ).value );
}

void emit_move( arch_emitter** mce, loperand d, loperand s ){
	assert( d.islocal );	
	do_assign( REF, loperand_to_operand( REF, d ).value, loperand_to_operand( REF,s ).value );
}

void emit_add( arch_emitter** mce, loperand d, loperand s, loperand t ){
	bop( REF, d, s, t, MOP_SPECIAL_ADDU, MOP_SPECIAL );
}

void emit_sub( arch_emitter** mce, loperand d, loperand s, loperand t ){
	bop( REF, d, s, t, MOP_SPECIAL_SUBU, MOP_SPECIAL );
}

void emit_mul( arch_emitter** mce, loperand d, loperand s, loperand t ){
	bop( REF, d, s, t, MOP_SPECIAL2_MUL, MOP_SPECIAL2 );
}

static void do_div( arch_emitter **mce, loperand d, loperand s, loperand t, bool islow  ){
	operand nil = OP_TARGETREG( _zero );	// dst is in hi and lo see instruction encoding
	operand dst = loperand_to_operand( REF, d ).value;
	do_bop( REF, nil, loperand_to_operand( REF, s ).value, loperand_to_operand( REF, t ).value,
			 MOP_SPECIAL_DIV, MOP_SPECIAL );

	// load the quotient into the dest
	operand src = OP_TARGETREG( dst.tag == OT_REG ? dst.reg : TEMP_REG1 );
	ENCODE_OP( REF, GEN_MIPS_OPCODE_3REG( MOP_SPECIAL, _zero, _zero, src.reg, islow ? MOP_SPECIAL_MFLO : MOP_SPECIAL_MFHI ) );

	if( dst.tag != OT_REG )
		do_assign( REF, dst, src );	
}

void emit_div( arch_emitter** mce, loperand d, loperand s, loperand t ){
	do_div( mce, d, s, t, true );
}

void emit_mod( arch_emitter** mce, loperand d, loperand s, loperand t ){
	do_div( mce, d, s, t, false );
}

void emit_forprep( arch_emitter** mce, loperand init, int pc, int j ){
	assert( init.islocal );
	
	loperand limit = { .islocal = true, .index = init.index + 1 };
	loperand step = { .islocal = true, .index = init.index + 2 };
	
	pc = pc + 1;
	emit_sub( mce, init, init, step );
	
	/* branch loop */
	push_branch( REF, pc + j );
	ENCODE_OP( REF,	GEN_MIPS_OPCODE_2REG( MOP_BEQ, _zero, _zero, pc + j ) );
	ENCODE_OP( REF, MOP_NOP );
}

void emit_forloop( arch_emitter** mce, loperand loopvar, int pc, int j ){
	assert( loopvar.islocal );

	loperand limit = { .islocal = true, .index = loopvar.index + 1 };
	loperand step = { .islocal = true, .index = loopvar.index + 2 };
	loperand iloopvar = { .islocal = true, .index = loopvar.index + 3 };	

	pc = pc + 1;

	emit_add( mce, loopvar, loopvar, step );
	emit_move( mce, iloopvar, loopvar );
	
	//| add a, a, a + ( 2 * 4 )
	//| move a + ( 3 * 4 ), a 
	//| sub v0, a, a + 4  
	//| bgtz v0, 1 
	//| jmp ( pc + j ) 
	//| nop 

	operand dst = { .tag = OT_REG, { .reg = TEMP_REG1 } };
	do_bop( REF, dst, loperand_to_operand( REF, loopvar ).value, 
			loperand_to_operand( REF, limit ).value, MOP_SPECIAL_SUBU, MOP_SPECIAL );

	
	ENCODE_OP( REF, GEN_MIPS_OPCODE_2REG( MOP_BGTZ, TEMP_REG1, 0, 3 ) );
	ENCODE_OP( REF, MOP_NOP );
	
	push_branch( REF, pc + j );
	ENCODE_OP( REF, GEN_MIPS_OPCODE_2REG( MOP_BEQ, _zero, _zero, pc + j ) );
	ENCODE_OP( REF, MOP_NOP );
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


void emit_newtable( arch_emitter** mce, loperand dst, int array, int hash ){
	loadim( REF, _a0, array );
	loadim( REF, _a1, hash );
	call_fn( REF, (uintptr_t)&table_create, 0 );

	operand src = OP_TARGETREG( _v0 ); 
	do_assign( REF, loperand_to_operand( REF, dst ).value,  src );
}

void emit_setlist( arch_emitter** mce, loperand table, int n, int block ){
	assert( table.islocal );

	operand dst = OP_TARGETREG( _a0 );
	operand val = OP_TARGETREG( _a3 );

	const int offset = ( block - 1 ) * LFIELDS_PER_FLUSH; 
	
	for( int i=1; i <= n; i++ ){

		loperand lval = { .islocal = true, .index = table.index + i };	

		do_assign( REF, dst, loperand_to_operand( REF, table ).value );
		loadim( REF, _a1, offset + i );
		loadim( REF, _a2, 0 );	// TODO: type
		do_assign( REF, val, loperand_to_operand( REF, lval ).value );
	
		call_fn( REF, (uintptr_t)&table_set, 0 );	

	}



}

void emit_gettable( arch_emitter** mce, loperand dst, loperand table, loperand idx ){
	operand t = OP_TARGETREG( _a0 );
	operand i = OP_TARGETREG( _a1 );
	operand src = OP_TARGETREG( _v0 ); 

	do_assign( REF, t, loperand_to_operand( REF, table ).value );
	do_assign( REF, i, loperand_to_operand( REF, idx ).value );
	// TODO: type

	call_fn( REF, (uintptr_t)&table_get, 0 );

	do_assign( REF, loperand_to_operand( REF, dst ).value,  src );

}

void emit_closure( arch_emitter** mce, loperand dst, struct proto* p ){
	// TODO: load the current closure into the _a1
//	loadim( REF, _a0, (uintptr_t)p );
//	call_fn( REF, (uintptr_t)&closure_create, 0 );


	operand src = OP_TARGETREG( TEMP_REG1 ); 
	loadim( REF, TEMP_REG1, (uintptr_t)p->code_start );
	do_assign( REF, loperand_to_operand( REF, dst ).value, src );
}


void emit_call( arch_emitter** mce, loperand closure, int nr_params, int nr_results ){
	assert( closure.islocal );

	struct arch_emitter* me = REF;

	// store live registers 
	store_frame( me );	

	// load args
	operand arg_clo = OP_TARGETREG( _a0 ); 
	do_assign( REF, arg_clo, loperand_to_operand( REF, closure ).value );	

#if 0	// this way requires the function to know the nr_locals in this frame
	loadim( REF, _a1, (uintptr_t)closure.index );
#else
	vreg_operand addr = llocal_to_stack_operand( REF, closure.index ); 
//	vreg_compiletime_stack_value( me->nr_locals, closure.index );
	EMIT( MI_ADDIU( _a1, _sp, addr.value.offset ) );
#endif

	if( nr_params > 0 )	// if zero returning function will load it 
		loadim( REF, _a2, (uintptr_t)nr_params - 1 );	 


	loadim( REF, _a3, (uintptr_t)nr_results );

	// call
	EMIT( MI_JALR( _a0 ) );
	EMIT( MI_NOP() );

	// reload registers ( including constants ) 
	load_frame( me );
}

/*
* Platfrom depdendent code ( TODO: the above should be plat indepedent )
*/

void arch_move( struct arch_emitter* me, operand d, operand s ){
	do_assign( me, d, s );
}

