/*
* (C) Iain Fraser - GPLv3 
*
* Lua platform independent branch instructions.
*/

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
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
#include "lstring.h"
#include "lmetaevent.h"
#include "lerror.h"
#include "macros.h"

#define REF	( *( struct emitter**)mce )

enum REL { REL_LT, REL_LEQ, REL_EQ };


static lua_Number do_lt( struct TValue* s, struct TValue* t ){
	struct TValue d;

	if( tag( s ) == LUA_TSTRING && tag( t ) == LUA_TSTRING )
		return lstrcmp( s, t ) < 0;			
	else if( call_binmevent( s, t, &d, TM_LT ) )
		return !tvisfalse( &d );
	else
		lcurrent_error( LE_ORDER );

	unreachable();
}
 
static lua_Number do_leq( struct TValue* s, struct TValue* t ){
	struct TValue d;

	if( tag( s ) == LUA_TSTRING && tag( t ) == LUA_TSTRING )
		return lstrcmp( s, t ) <= 0;			
	else if( call_binmevent( s, t, &d, TM_LE ) )
		return !tvisfalse( &d );
	else if( call_binmevent( t, s, &d, TM_LT ) )
		return tvisfalse( &d );
	else
		lcurrent_error( LE_ORDER );

	unreachable();
}

static lua_Number do_eq( struct TValue* s, struct TValue* t ){
	struct TValue *mt,d;

	if( tag( s ) != tag( t ) )
		return false;

	if( tag( s ) == LUA_TSTRING && tag( t ) == LUA_TSTRING )
		return lstrcmp( s, t ) == 0;

	switch( tag( s ) ){
		case LUA_TNIL:
			return true;
		case LUA_TNUMBER:
			return tvtonum( s ) == tvtonum( t );
		case LUA_TBOOLEAN:
			return tvtobool( s ) == tvtobool( t );
		case LUA_TLIGHTUSERDATA:
			return tvtolud( s ) == tvtolud( t );
		case LUA_TLCF:
			return tvtolcf( s ) == tvtolcf( t );
		case LUA_TUSERDATA:
		case LUA_TTABLE:
			if( tvtogch( s ) == tvtogch( t ) )
				return true;

			if( ( mt = getmt( s ) ) != getmt( t ) )
				return false;	
			break;
		default:
			return tvtogch( s ) == tvtogch( t ); 
	}

	if( mt && mt_call_binmevent( mt, s, t, &d, TM_EQ ) )
		return tvisfalse( &d );
	
	return false;
}

static lua_Number ljc_relational( lua_Number st, lua_Number sv
					, lua_Number tt, lua_Number tv
					, int op ) {
	assert( !( st == LUA_TNUMBER && tt == LUA_TNUMBER ) );
	
	struct TValue s = { .t = st, .v = (union Value)sv };
	struct TValue t = { .t = tt, .v = (union Value)tv };
	
	switch( op ){
		case REL_LT:
			return do_lt( &s, &t ); 
		case REL_LEQ:
			return do_leq( &s, &t );
		case REL_EQ:
			return do_eq( &s, &t );
		default:
			assert( false );	
	}
}


typedef void (*arch_rel)( struct emitter*, struct machine*
					, operand, operand, label );

static void emit_relational( struct emitter *me, struct machine_ops *mop
					, struct frame* f 
					, loperand s, loperand t
					, arch_rel ar, int op
					, bool expect ){

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
	mop->beq( me, f->m, tag, OP_TARGETIMMED( expect ), l );
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
	if( !pred )
		emit_relational( REF, mop, f, a, b, mop->beq, REL_EQ, true  );
	else
		emit_relational( REF, mop, f, a, b, mop->bne, REL_EQ, false );
}

void emit_lt( struct emitter** mce, struct machine_ops* mop
					, struct frame *f
					, loperand a
					, loperand b
					, int pred ) {
	if( !pred )
		emit_relational( REF, mop, f, a, b, mop->blt, REL_LT, true  );
	else
		emit_relational( REF, mop, f, a, b, mop->bgt, REL_LT, false );
}	

void emit_le( struct emitter** mce, struct machine_ops* mop
					, struct frame *f
					, loperand a
					, loperand b
					, int pred ) {
	if( !pred )
		emit_relational( REF, mop, f, a, b, mop->ble, REL_LEQ, true );
	else
		emit_relational( REF, mop, f, a, b, mop->bge, REL_LEQ, false );
}
 
