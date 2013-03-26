/*
* (C) Iain Fraser - GPLv3 
*
* Lua platform independent branch instructions.
*/

#include <stdio.h>
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
#include "frame.h"
#include "synthetic.h"

#define REF	( *( struct emitter**)mce )

enum REL { REL_ADD, BOP_SUB, BOP_DIV, BOP_MUL, BOP_MOD, BOP_POW };

static lua_Number ljc_relational( lua_Number st, lua_Number sv
					, lua_Number tt, lua_Number tv
					, int op ) {
	// TODO: error function
	printf("relational\n");	
	return true;
}


typedef void (*arch_rel)( struct emitter*, struct machine*
					, operand, operand, label );

static void emit_relational( struct emitter *me, struct machine_ops *mop
					, struct frame* f 
					, loperand s, loperand t
					, arch_rel ar, int op ){

	vreg_operand os = loperand_to_operand( f, s ),
			ot = loperand_to_operand( f, t );

	unsigned int pc = me->ops->pc( me ) + 2;
	label l = LBL_PC( pc );

	// determine if coercion is required 
	operand tag = OP_TARGETREG( acquire_temp( mop, me, f->m ) );
	mop->bor( me, f->m, tag, os.type, ot.type );
	mop->beq( me, f->m, tag, OP_TARGETIMMED( 0 ), LBL_NEXT( 0 ) );

	// do coercion 
	mop->call_static_cfn( me, f, (uintptr_t)&ljc_relational
					, &tag, 5, os.type, os.value 
					, ot.type, ot.value
					, OP_TARGETIMMED( op ) ); 
	mop->beq( me, f->m, tag, OP_TARGETIMMED( true ), l );
	mop->b( me, f->m, LBL_NEXT( 1 ) );	

	// do primitive relational  
	me->ops->label_local( me, 0 );
	ar( me, f->m, os.value, ot.value, l );
	me->ops->label_local( me, 1 );
	

	release_temp( mop, me, f->m );	

	return;
}

void emit_jmp( struct emitter** mce, struct machine_ops* mop
					, struct frame *f
					, loperand a
					, int offset ){
	assert( a.islocal );

	// if not zero then any upvalues below the vreg need to be closed.
	if( a.index > 0 ){
		vreg_operand op = vreg_to_operand( f, a.index + 1, true );
		operand base = OP_TARGETREG( acquire_temp( mop, REF, f->m ) );
		address_of( mop, REF, f->m, base, op.type );  
		mop->call_static_cfn( REF, f, (uintptr_t)&closure_close, NULL
				, 1
				, base ); 
		release_temp( mop, REF, f->m ); 
	}
	
	unsigned int pc = (int)REF->ops->pc( REF ) + offset + 1;
	mop->b( REF, f->m, LBL_PC( pc ) ); 
} 


void emit_eq( struct emitter** mce, struct machine_ops* mop
					, struct frame *f
					, loperand a
					, loperand b
					, int pred ) {
	vreg_operand va = loperand_to_operand( f, a );
	vreg_operand vb = loperand_to_operand( f, b );
 
#if 0	// TODO: compare type && value
	// TODO: verify there both numbers
	unsigned int pc = REF->ops->pc( REF ) + 2;
	if( pred )
		mop->ble( REF, f->m, va.value, vb.value, LBL_PC( pc ) ); 
	else
		mop->bge( REF, f->m, va.value, vb.value, LBL_PC( pc ) ); 
#endif
}

void emit_lt( struct emitter** mce, struct machine_ops* mop
					, struct frame *f
					, loperand a
					, loperand b
					, int pred ) {

	if( !pred )
		emit_relational( REF, mop, f, a, b, mop->blt, 0 );
	else
		emit_relational( REF, mop, f, a, b, mop->bgt, 0 );
	return;

	vreg_operand va = loperand_to_operand( f, a );
	vreg_operand vb = loperand_to_operand( f, b );
 
	/*
	* TODO: if number do this, if string do lex, if table try meta,
	* otherwise fail.
	*/
	unsigned int pc = REF->ops->pc( REF ) + 2;
	if( !pred )
		mop->blt( REF, f->m, va.value, vb.value, LBL_PC( pc ) ); 
	else
		mop->bgt( REF, f->m, va.value, vb.value, LBL_PC( pc ) ); 
}	

void emit_le( struct emitter** mce, struct machine_ops* mop
					, struct frame *f
					, loperand a
					, loperand b
					, int pred ) {
	if( !pred )
		emit_relational( REF, mop, f, a, b, mop->ble, 0 );
	else
		emit_relational( REF, mop, f, a, b, mop->bge, 0 );
	
#if 0
	vreg_operand va = loperand_to_operand( f, a );
	vreg_operand vb = loperand_to_operand( f, b );
 
	/*
	* TODO: if number do this, if string do lex, if table try meta,
	* otherwise fail.
	*/
	unsigned int pc = REF->ops->pc( REF ) + 2;
	if( !pred )
		mop->ble( REF, f->m, va.value, vb.value, LBL_PC( pc ) ); 
	else
		mop->bge( REF, f->m, va.value, vb.value, LBL_PC( pc ) ); 
#endif
}
 
