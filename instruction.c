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
#include "xlogue.h"

// Actual includes
#include "frame.h"
#include "synthetic.h"



#define REF	( *( struct emitter**)mce ) 

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


typedef void (*arch_bop)( struct emitter*, struct machine* m, operand, operand, operand );

static void emit_bop( struct emitter* me, struct frame* f, loperand d, loperand s, loperand t, arch_bop ab ){
	vreg_operand od = loperand_to_operand( f, d ),
			os = loperand_to_operand( f, s ),
			ot = loperand_to_operand( f, t );

	// TODO: verify there all numbers 

	ab( me, f->m, od.value, os.value, ot.value );
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
	mop->b( REF, f->m, LBL_PC( pc + j ) ); 
}

void emit_forloop( struct emitter** mce, struct machine_ops* mop, struct frame* f, loperand loopvar, int pc, int j ){
	assert( loopvar.islocal );

	loperand limit = { .islocal = true, .index = loopvar.index + 1 };
	loperand step = { .islocal = true, .index = loopvar.index + 2 };
	loperand iloopvar = { .islocal = true, .index = loopvar.index + 3 };	

	pc = pc + 1;

	//| add a, a, a + ( 2 * 4 )
	//| move a + ( 3 * 4 ), a 
	//| sub v0, a, a + 4  
	//| bgtz v0, 1 
	//| jmp ( pc + j ) 
	//| nop 
	// TODO: check for type on bgt 
	emit_add( mce, mop, f, loopvar, loopvar, step );
	emit_move( mce, mop, f, iloopvar, loopvar );
	mop->bgt( REF, f->m, loperand_to_operand( f, loopvar ).value, loperand_to_operand( f, limit ).value, LBL_NEXT( 0 ) );
	mop->b( REF, f->m, LBL_PC( pc + j ) );	
	REF->ops->label_local( REF, 0 );
}




void emit_newtable( struct emitter** mce, struct machine_ops* mop, struct frame* f, loperand dst, int array, int hash ){
	loadim( mop, REF, f->m, _a0, array );
	loadim( mop, REF, f->m, _a1, hash );
	mop->call_cfn( REF, f->m, (uintptr_t)&table_create, 0 );

	operand src = OP_TARGETREG( _v0 ); 
	mop->move( REF, f->m, loperand_to_operand( f, dst ).value,  src );
}

void emit_setlist( struct emitter** mce, struct machine_ops* mop, struct frame* f, loperand table, int n, int block ){
	assert( table.islocal );

	operand dst = OP_TARGETREG( _a0 );
	operand val = OP_TARGETREG( _a3 );

	const int offset = ( block - 1 ) * LFIELDS_PER_FLUSH; 
	
	for( int i=1; i <= n; i++ ){

		loperand lval = { .islocal = true, .index = table.index + i };	

		mop->move( REF, f->m, dst, loperand_to_operand( f, table ).value );
		loadim( mop, REF, f->m, _a1, offset + i );
		loadim( mop, REF, f->m, _a2, 0 );	// TODO: type
		mop->move( REF, f->m, val, loperand_to_operand( f, lval ).value );
	
		mop->call_cfn( REF, f->m, (uintptr_t)&table_set, 0 );	

	}



}

void emit_gettable( struct emitter** mce, struct machine_ops* mop, struct frame* f, loperand dst, loperand table, loperand idx ){
	operand t = OP_TARGETREG( _a0 );
	operand i = OP_TARGETREG( _a1 );
	operand src = OP_TARGETREG( _v0 ); 

	mop->move( REF, f->m, t, loperand_to_operand( f, table ).value );
	mop->move( REF, f->m, i, loperand_to_operand( f, idx ).value );
	// TODO: type

	mop->call_cfn( REF, f->m, (uintptr_t)&table_get, 0 );

	mop->move( REF, f->m, loperand_to_operand( f, dst ).value,  src );

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

	// temp - do proper work later
	mop->move( REF, f->m, loperand_to_operand( f, dst ).value, OP_TARGETIMMED( (uintptr_t)p->code_start ) ); 

}


void emit_call( struct emitter** mce, struct machine_ops* mop, struct frame* f, loperand closure, int nr_params, int nr_results ){
	assert( closure.islocal );

	store_frame( mop, REF, f );	
	do_call( mop, REF, f, closure.index, nr_params, nr_results );
	load_frame( mop, REF, f );
}


void emit_ret( struct emitter** mce, struct machine_ops* mop, struct frame* f, loperand base, int nr_results ){
#if 0
	int j = ( REF->epi - ( REF->size + 1 ) ) / 4;	// branch is from delay slot
	ENCODE_OP( REF, GEN_MIPS_OPCODE_2REG( MOP_BEQ, _zero, _zero, (int16_t)j ) );
	ENCODE_OP( REF, MOP_NOP );
#endif
	do_ret( mop, REF, f, base.index, nr_results );
// 	mop->b( REF, f->m, LBL_EC( f->epi ) ); 
} 



