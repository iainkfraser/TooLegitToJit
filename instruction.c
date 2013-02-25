/*
* (C) Iain Fraser - GPLv3 
*
* 1:1 mapping of LuaVM instructions
* TODO: move from MIPS dependent to arch indepedent 
*/

#include <assert.h>
#include <stdlib.h>
#include "arch/mips/regdef.h"
#include "bit_manip.h"
#include "instruction.h"
#include "operand.h"
#include "stack.h"
#include "table.h"
#include "func.h"
#include "emitter.h"
#include "machine.h"
#include "lopcodes.h"

// Actual includes
#include "frame.h"
#include "synthetic.h"



#define REF	( *( struct emitter**)mce ) 

void emit_ret( struct emitter** mce, struct machine_ops* mop, struct frame* f ){
#if 0
	int j = ( REF->epi - ( REF->size + 1 ) ) / 4;	// branch is from delay slot
	ENCODE_OP( REF, GEN_MIPS_OPCODE_2REG( MOP_BEQ, _zero, _zero, (int16_t)j ) );
	ENCODE_OP( REF, MOP_NOP );
#endif 
} 


void emit_loadk( struct emitter** mce, struct machine_ops* mop, struct frame* f, int l, int k ){
	assign( mop, REF, f->m, llocal_to_operand( f, l ), lconst_to_operand( f, k ) );
}

void emit_move( struct emitter** mce, struct machine_ops* mop, struct frame* f, loperand d, loperand s ){
	assert( d.islocal );	
	assign( mop, REF, f->m, loperand_to_operand( f, d ), loperand_to_operand( f,s ) );
}


/*
* Binary operators 
*/


typedef void (*arch_bop)( struct emitter*, operand, operand, operand );

static void emit_bop( struct emitter* me, struct frame* f, loperand d, loperand s, loperand t, arch_bop ab ){
	vreg_operand od = loperand_to_operand( f, d ),
			os = loperand_to_operand( f, s ),
			ot = loperand_to_operand( f, t );

	// TODO: verify there all numbers 

	ab( me, od.value, os.value, ot.value );
}

void emit_add( struct emitter** mce, struct machine_ops* mop, struct frame* f, loperand d, loperand s, loperand t ){
	emit_bop( REF, f, d, s, t, mop->add );
}

void emit_sub( struct emitter** mce, struct machine_ops* mop, struct frame* f, loperand d, loperand s, loperand t ){
	emit_bop( REF, f, d, s, t, mop->sub );
}

void emit_mul( struct emitter** mce, struct machine_ops* mop, struct frame* f, loperand d, loperand s, loperand t ){
	emit_bop( REF, f, d, s, t, mop->mul );
}

void emit_div( struct emitter** mce, struct machine_ops* mop, struct frame* f, loperand d, loperand s, loperand t ){
	emit_bop( REF, f, d, s, t, mop->div );
}

void emit_mod( struct emitter** mce, struct machine_ops* mop, struct frame* f, loperand d, loperand s, loperand t ){
	emit_bop( REF, f, d, s, t, mop->mod );
}

/*
* For loop 
*/

void emit_forprep( struct emitter** mce, struct machine_ops* mop, struct frame* f, loperand init, int pc, int j ){
	assert( init.islocal );
	
	loperand limit = { .islocal = true, .index = init.index + 1 };
	loperand step = { .islocal = true, .index = init.index + 2 };
	
	pc = pc + 1;
	emit_sub( mce, mop, f, init, init, step );
	
	/* branch loop */
#if 0
	push_branch( REF, pc + j );
	ENCODE_OP( REF,	GEN_MIPS_OPCODE_2REG( MOP_BEQ, _zero, _zero, pc + j ) );
	ENCODE_OP( REF, MOP_NOP );
#endif
}

void emit_forloop( struct emitter** mce, struct machine_ops* mop, struct frame* f, loperand loopvar, int pc, int j ){
	assert( loopvar.islocal );

	loperand limit = { .islocal = true, .index = loopvar.index + 1 };
	loperand step = { .islocal = true, .index = loopvar.index + 2 };
	loperand iloopvar = { .islocal = true, .index = loopvar.index + 3 };	

	pc = pc + 1;

	emit_add( mce, mop, f, loopvar, loopvar, step );
	emit_move( mce, mop, f, iloopvar, loopvar );
	
	//| add a, a, a + ( 2 * 4 )
	//| move a + ( 3 * 4 ), a 
	//| sub v0, a, a + 4  
	//| bgtz v0, 1 
	//| jmp ( pc + j ) 
	//| nop 

#if 0
	operand dst = { .tag = OT_REG, { .reg = TEMP_REG1 } };
	mop->sub( REF, dst, loperand_to_operand( f, loopvar ).value, loperand_to_operand( f, limit ).value );	

#endif
//	do_bop( REF, dst, loperand_to_operand( f, loopvar ).value, 
//			loperand_to_operand( f, limit ).value, MOP_SPECIAL_SUBU, MOP_SPECIAL );

#if 0
	ENCODE_OP( REF, GEN_MIPS_OPCODE_2REG( MOP_BGTZ, TEMP_REG1, 0, 3 ) );
	ENCODE_OP( REF, MOP_NOP );
	
	push_branch( REF, pc + j );
	ENCODE_OP( REF, GEN_MIPS_OPCODE_2REG( MOP_BEQ, _zero, _zero, pc + j ) );
	ENCODE_OP( REF, MOP_NOP );
#endif
}




void emit_newtable( struct emitter** mce, struct machine_ops* mop, struct frame* f, loperand dst, int array, int hash ){
	loadim( mop, REF, f->m, _a0, array );
	loadim( mop, REF, f->m, _a1, hash );
	mop->call_cfn( REF, (uintptr_t)&table_create, 0 );

	operand src = OP_TARGETREG( _v0 ); 
	mop->move( REF, loperand_to_operand( f, dst ).value,  src );
}

void emit_setlist( struct emitter** mce, struct machine_ops* mop, struct frame* f, loperand table, int n, int block ){
	assert( table.islocal );

	operand dst = OP_TARGETREG( _a0 );
	operand val = OP_TARGETREG( _a3 );

	const int offset = ( block - 1 ) * LFIELDS_PER_FLUSH; 
	
	for( int i=1; i <= n; i++ ){

		loperand lval = { .islocal = true, .index = table.index + i };	

		mop->move( REF, dst, loperand_to_operand( f, table ).value );
		loadim( mop, REF, f->m, _a1, offset + i );
		loadim( mop, REF, f->m, _a2, 0 );	// TODO: type
		mop->move( REF, val, loperand_to_operand( f, lval ).value );
	
		mop->call_cfn( REF, (uintptr_t)&table_set, 0 );	

	}



}

void emit_gettable( struct emitter** mce, struct machine_ops* mop, struct frame* f, loperand dst, loperand table, loperand idx ){
	operand t = OP_TARGETREG( _a0 );
	operand i = OP_TARGETREG( _a1 );
	operand src = OP_TARGETREG( _v0 ); 

	mop->move( REF, t, loperand_to_operand( f, table ).value );
	mop->move( REF, i, loperand_to_operand( f, idx ).value );
	// TODO: type

	mop->call_cfn( REF, (uintptr_t)&table_get, 0 );

	mop->move( REF, loperand_to_operand( f, dst ).value,  src );

}

void emit_closure( struct emitter** mce, struct machine_ops* mop, struct frame* f, loperand dst, struct proto* p ){
	// TODO: load the current closure into the _a1
//	loadim( REF, _a0, (uintptr_t)p );
//	call_fn( REF, (uintptr_t)&closure_create, 0 );

#if 0
	operand src = OP_TARGETREG( TEMP_REG1 ); 
	loadim( mop, REF, f->m, TEMP_REG1, (uintptr_t)p->code_start );
	mop->move(  REF, loperand_to_operand( f, dst ).value, src );
#endif 
}


void emit_call( struct emitter** mce, struct machine_ops* mop, struct frame* f, loperand closure, int nr_params, int nr_results ){
	assert( closure.islocal );

	 struct emitter* me = REF;

	// store live registers 
	store_frame( me, f );	

	// load args
	operand arg_clo = OP_TARGETREG( _a0 ); 
	mop->move( REF, arg_clo, loperand_to_operand( f, closure ).value );	


#if 0	// this way requires the function to know the nr_locals in this frame
	loadim( REF, _a1, (uintptr_t)closure.index );
#else
	vreg_operand addr = llocal_to_stack_operand( f, closure.index ); 
//	vreg_compiletime_stack_value( me->nr_locals, closure.index );
#if 0
	EMIT( MI_ADDIU( _a1, _sp, addr.value.offset ) );
#endif
#endif

	if( nr_params > 0 )	// if zero returning function will load it 
		loadim( mop, REF, f->m, _a2, (uintptr_t)nr_params - 1 );	 


	loadim( mop, REF, f->m, _a3, (uintptr_t)nr_results );

	// call
#if 0
	EMIT( MI_JALR( _a0 ) );
	EMIT( MI_NOP() );
#endif

	// reload registers ( including constants ) 
	load_frame( me, f );
}
