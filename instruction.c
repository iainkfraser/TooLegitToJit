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
#include "jitfunc.h"

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
	emit_bop( REF, f, d, s, t, mop->sdiv );
}

void emit_mod( struct emitter** mce, struct machine_ops* mop, struct frame* f, loperand d, loperand s, loperand t ){
	emit_bop( REF, f, d, s, t, mop->smod );
	
	// TODO: convert to floored mod 
	// if ((d > 0 && t < 0) || (d < 0 && t > 0)) d = d+t;
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
	operand res = loperand_to_operand( f, dst ).value;
	mop->call_static_cfn( REF, f, (uintptr_t)&table_create, &res, 2, OP_TARGETIMMED( array ), OP_TARGETIMMED( hash ) );
}

void emit_setlist( struct emitter** mce, struct machine_ops* mop, struct frame* f, loperand table, int n, int block ){
	assert( table.islocal );
	assert( block != 0 );		// TODO: read the next instruction for int size
	assert( n != 0 );		// TODO: sub top of stack to get # of args

	const int offset = ( block - 1 ) * LFIELDS_PER_FLUSH; 
	vreg_operand t = loperand_to_operand( f, table );
	vreg_operand v = vreg_to_operand( f, table.index + 1, true );

	save_frame_limit( mop, REF, f, table.index + 1, n );
	
	// TODO: verify its table

	// get base adress
	operand base = OP_TARGETREG( acquire_temp( mop, REF, f->m ) );
	mop->add( REF, f->m, base, OP_TARGETREG( v.value.base ), OP_TARGETIMMED( v.value.offset  ) );
	mop->call_static_cfn( REF, f, (uintptr_t)&table_setlist, NULL, 4,
		t.value, base, OP_TARGETIMMED( offset ), OP_TARGETIMMED( n ) );
	release_temp( mop, REF, f->m );

#if 0	
	for( int i=1; i <= n; i++ ){

		loperand lval = { .islocal = true, .index = table.index + i };	

		mop->move( REF, f->m, dst, loperand_to_operand( f, table ).value );
		loadim( mop, REF, f->m, _a1, offset + i );
		loadim( mop, REF, f->m, _a2, 0 );	// TODO: type
		mop->move( REF, f->m, val, loperand_to_operand( f, lval ).value );
	
		mop->call_static_cfn( REF, f, (uintptr_t)&table_set, NULL, 0 );	

	}
#endif



}

void emit_gettable( struct emitter** mce, struct machine_ops* mop, struct frame* f, loperand dst, loperand table, loperand idx ){

	// TODO: verify its a table and number
	operand t = loperand_to_operand( f, table ).value;
	operand i = loperand_to_operand( f, idx ).value;
	operand d = loperand_to_operand( f, dst ).value;

	// TODO: get return type by pointer arg
	mop->call_static_cfn( REF, f, (uintptr_t)&table_get, &d, 2, t, i );


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

#if 0
//	store_frame( mop, REF, f );	
#else
	jfunc_call( mop, REF, f->m, JF_STORE_LOCALS, jf_storelocal_offset( f->m, f->nr_locals ), JFUNC_UNLIMITED_STACK, 0 );
#endif 

	do_call( mop, REF, f, closure.index, nr_params, nr_results );
#if 0 
	load_frame( mop, REF, f );
#else
	jfunc_call( mop, REF, f->m, JF_LOAD_LOCALS, jf_loadlocal_offset( f->m, f->nr_locals ), JFUNC_UNLIMITED_STACK, 0 );
#endif
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



