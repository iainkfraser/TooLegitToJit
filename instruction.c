/*
* (C) Iain Fraser - GPLv3 
*
* 1:1 mapping of LuaVM instructions
* TODO: move from MIPS dependent to arch indepedent 
*/

#include <stdlib.h>
#include "arch/mips/regdef.h"
#include "arch/mips/opcodes.h"
#include "arch/mips/emitter.h"
#include "arch/mips/vstack.h"
#include "arch/mips/regmap.h"
#include "bit_manip.h"
#include "instruction.h"
#include "operand.h"
#include "stack.h"
#include "table.h"
#include "func.h"

#define REF	( *( arch_emitter**)mce ) 

void emit_ret( struct arch_emitter** mce ){
	int j = ( REF->epi - ( REF->size + 1 ) ) / 4;	// branch is from delay slot
	ENCODE_OP( REF, GEN_MIPS_OPCODE_2REG( MOP_BEQ, _zero, _zero, (int16_t)j ) );
	ENCODE_OP( REF, MOP_NOP );
} 


void emit_loadk( struct arch_emitter** mce, int l, int k ){
	assign( REF, llocal_to_operand( REF, l ), lconst_to_operand( REF, k ) );
}

void emit_move( struct arch_emitter** mce, loperand d, loperand s ){
	assert( d.islocal );	
	assign( REF, loperand_to_operand( REF, d ), loperand_to_operand( REF,s ) );
}


/*
* Binary operators 
*/


typedef void (*arch_bop)( struct arch_emitter*, operand, operand, operand );

static void emit_bop( struct arch_emitter* me, loperand d, loperand s, loperand t, arch_bop ab ){
	vreg_operand od = loperand_to_operand( me, d ),
			os = loperand_to_operand( me, s ),
			ot = loperand_to_operand( me, t );

	// TODO: verify there all numbers 

	ab( me, od.value, os.value, ot.value );
}

void emit_add( struct arch_emitter** mce, loperand d, loperand s, loperand t ){
	emit_bop( REF, d, s, t, &arch_add );
}

void emit_sub( struct arch_emitter** mce, loperand d, loperand s, loperand t ){
	emit_bop( REF, d, s, t, &arch_sub );
}

void emit_mul( struct arch_emitter** mce, loperand d, loperand s, loperand t ){
	emit_bop( REF, d, s, t, &arch_mul );
}

void emit_div( struct arch_emitter** mce, loperand d, loperand s, loperand t ){
	emit_bop( REF, d, s, t, &arch_div );
}

void emit_mod( struct arch_emitter** mce, loperand d, loperand s, loperand t ){
	emit_bop( REF, d, s, t, &arch_mod );
}

/*
* For loop 
*/

void emit_forprep( struct arch_emitter** mce, loperand init, int pc, int j ){
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

void emit_forloop( struct arch_emitter** mce, loperand loopvar, int pc, int j ){
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
	arch_sub( REF, dst, loperand_to_operand( REF, loopvar ).value, loperand_to_operand( REF, limit ).value );	
//	do_bop( REF, dst, loperand_to_operand( REF, loopvar ).value, 
//			loperand_to_operand( REF, limit ).value, MOP_SPECIAL_SUBU, MOP_SPECIAL );

	
	ENCODE_OP( REF, GEN_MIPS_OPCODE_2REG( MOP_BGTZ, TEMP_REG1, 0, 3 ) );
	ENCODE_OP( REF, MOP_NOP );
	
	push_branch( REF, pc + j );
	ENCODE_OP( REF, GEN_MIPS_OPCODE_2REG( MOP_BEQ, _zero, _zero, pc + j ) );
	ENCODE_OP( REF, MOP_NOP );
}




void emit_newtable( struct arch_emitter** mce, loperand dst, int array, int hash ){
	loadim( REF, _a0, array );
	loadim( REF, _a1, hash );
	arch_call_cfn( REF, (uintptr_t)&table_create, 0 );

	operand src = OP_TARGETREG( _v0 ); 
	arch_move( REF, loperand_to_operand( REF, dst ).value,  src );
}

void emit_setlist( struct arch_emitter** mce, loperand table, int n, int block ){
	assert( table.islocal );

	operand dst = OP_TARGETREG( _a0 );
	operand val = OP_TARGETREG( _a3 );

	const int offset = ( block - 1 ) * LFIELDS_PER_FLUSH; 
	
	for( int i=1; i <= n; i++ ){

		loperand lval = { .islocal = true, .index = table.index + i };	

		arch_move( REF, dst, loperand_to_operand( REF, table ).value );
		loadim( REF, _a1, offset + i );
		loadim( REF, _a2, 0 );	// TODO: type
		arch_move( REF, val, loperand_to_operand( REF, lval ).value );
	
		arch_call_cfn( REF, (uintptr_t)&table_set, 0 );	

	}



}

void emit_gettable( struct arch_emitter** mce, loperand dst, loperand table, loperand idx ){
	operand t = OP_TARGETREG( _a0 );
	operand i = OP_TARGETREG( _a1 );
	operand src = OP_TARGETREG( _v0 ); 

	arch_move( REF, t, loperand_to_operand( REF, table ).value );
	arch_move( REF, i, loperand_to_operand( REF, idx ).value );
	// TODO: type

	arch_call_cfn( REF, (uintptr_t)&table_get, 0 );

	arch_move( REF, loperand_to_operand( REF, dst ).value,  src );

}

void emit_closure( struct arch_emitter** mce, loperand dst, struct proto* p ){
	// TODO: load the current closure into the _a1
//	loadim( REF, _a0, (uintptr_t)p );
//	call_fn( REF, (uintptr_t)&closure_create, 0 );


	operand src = OP_TARGETREG( TEMP_REG1 ); 
	loadim( REF, TEMP_REG1, (uintptr_t)p->code_start );
	arch_move( REF, loperand_to_operand( REF, dst ).value, src );
}


void emit_call( struct arch_emitter** mce, loperand closure, int nr_params, int nr_results ){
	assert( closure.islocal );

	 struct arch_emitter* me = REF;

	// store live registers 
	store_frame( me );	

	// load args
	operand arg_clo = OP_TARGETREG( _a0 ); 
	arch_move( REF, arg_clo, loperand_to_operand( REF, closure ).value );	

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
