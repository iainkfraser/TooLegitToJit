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

void emit_loadk( struct emitter** mce, struct machine_ops* mop, struct frame* f
							, int l, int k ){
	assign( mop, REF, f->m, llocal_to_operand( f, l )
				, lconst_to_operand( f, k ) );
}

void emit_move( struct emitter** mce, struct machine_ops* mop, struct frame* f
						, loperand d, loperand s ){
	assert( d.islocal );	
	assign( mop, REF, f->m, loperand_to_operand( f, d )
					, loperand_to_operand( f,s ) );
}


/*
* For loop 
*/

void emit_forprep( struct emitter** mce, struct machine_ops* mop
						, struct frame* f
						, loperand init
						, int pc, int j ){
	assert( init.islocal );
	
	loperand limit = { .islocal = true, .index = init.index + 1 };
	loperand step = { .islocal = true, .index = init.index + 2 };
	
	pc = pc + 1;
	emit_sub( mce, mop, f, init, init, step );
	mop->b( REF, f->m, LBL_PC( pc + j ) ); 
}

void emit_forloop( struct emitter** mce, struct machine_ops* mop
						, struct frame* f
						, loperand loopvar
						, int pc, int j ){
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


void emit_closure( struct emitter** mce, struct machine_ops* mop, struct frame* f, loperand dst, struct proto* p ){
	vreg_operand d = loperand_to_operand( f, dst );
	operand pproto = OP_TARGETIMMED( (uintptr_t)p );
	operand parentc = get_frame_closure( f );  
	operand stackbase = vreg_to_operand( f, 0, true ).type;

	operand sb = OP_TARGETREG( acquire_temp( mop, REF, f->m ) );
	mop->add( REF, f->m, sb, OP_TARGETREG( stackbase.base ), OP_TARGETIMMED( stackbase.offset ) );
	mop->call_static_cfn( REF, f, (uintptr_t)&closure_create, &d.value, 3, pproto, parentc, sb );
	release_temp( mop, REF, f->m );
	mop->move( REF, f->m, d.type, OP_TARGETIMMED( ctb( LUA_TLCL ) ) );
}


void emit_call( struct emitter** mce, struct machine_ops* mop, struct frame* f,
			 loperand closure, int nr_params, int nr_results ){
	assert( closure.islocal );

#if 0
//	store_frame( mop, REF, f );	
#else
	jfunc_call( mop, REF, f->m, JF_STORE_LOCALS
			, jf_storelocal_offset( f->m, f->nr_locals )
			, JFUNC_UNLIMITED_STACK
			, 0 );
#endif 

	do_call( mop, REF, f, closure.index, nr_params, nr_results );
#if 0 
	load_frame( mop, REF, f );
#else
	jfunc_call( mop, REF, f->m, JF_LOAD_LOCALS
			, jf_loadlocal_offset( f->m, f->nr_locals )
			, JFUNC_UNLIMITED_STACK
			, 0 );
#endif
}


void emit_ret( struct emitter** mce, struct machine_ops* mop, struct frame* f, loperand base, int nr_results ){
	do_ret( mop, REF, f, base.index, nr_results );
} 


