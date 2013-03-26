/*
* (C) Iain Fraser - GPLv3 
*
* Lua platform independent arithmetic instructions.
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

enum BOP { BOP_ADD, BOP_SUB, BOP_DIV, BOP_MUL, BOP_MOD, BOP_POW };

static lua_Number ljc_nonnumeric_bop( lua_Number st, lua_Number sv
					, lua_Number tt, lua_Number tv
					, int op, lua_Number* vt ) {
	if( st == LUA_TSTRING )
		sscanf( (char*)sv, "%d", &sv );

	if( tt == LUA_TSTRING )
		sscanf( (char*)tv, "%d", &tv );

	*vt = LUA_TNUMBER;
	switch( op )
	{
	case BOP_ADD:
		return sv + tv;
	case BOP_SUB:
		return sv - tv;
	case BOP_DIV:
		return sv / tv;
	case BOP_MUL:
		return sv * tv;
	case BOP_MOD:
		return sv % tv;
	case BOP_POW:
		return 0;
	default:
		assert( false );
	}
}

typedef void (*arch_bop)( struct emitter*, struct machine* m
						, operand, operand, operand );

/*
* Currently each bop takes multiple instruction mainly due to coercion
* function call. The coerccion step can be moved to a jfunction by
* encoding the vreg index of the d, s and t. Then using a function
* that loads/store an index. e.g. load( 3 ) would do a0 = s6.
* The encoding can be caclculated statically because vregs indexes
* are known at compile time. In fact the direct calculation can
* also be included in the JFunction. So if memory is an issue a single
* bop can be reduced to two instructions i.e.
*
* li code
* b  jf_coercion
*
* Where code woule be something like [ op | didx | tidx | sidx ]. The only
* issue is how to encode constants. Maybe have separate function to handle
* constants OR load them from the closure->prototype->consts BUT that requires
* constant not be freed after JIT.  
*/
static void emit_bop( struct emitter* me, struct machine_ops *mop
					, struct frame* f
					, loperand d, loperand s, loperand t
					, arch_bop ab, int op ){
	assert( d.islocal );

	vreg_operand od = loperand_to_operand( f, d ),
			os = loperand_to_operand( f, s ),
			ot = loperand_to_operand( f, t );

	// determine if coercion is required 
	operand tag = OP_TARGETREG( acquire_temp( mop, me, f->m ) );
	mop->bor( me, f->m, tag, os.type, ot.type );
	mop->beq( me, f->m, tag, OP_TARGETIMMED( 0 ), LBL_NEXT( 0 ) );

	// do coercion 
	address_of( mop, me, f->m, tag
			, vreg_to_operand( f, d.index, true ).type );
	mop->call_static_cfn( me, f, (uintptr_t)&ljc_nonnumeric_bop
					, &od.value, 6, os.type, os.value 
					, ot.type, ot.value
					, OP_TARGETIMMED( op ), tag ); 
	vreg_type_fill( mop, me, f, d.index );
	mop->b( me, f->m, LBL_NEXT( 1 ) );	

	// do direct binary operation 
	me->ops->label_local( me, 0 );
	ab( me, f->m, od.value, os.value, ot.value );
	mop->move( me, f->m, od.type, OP_TARGETIMMED( LUA_TNUMBER ) );
	me->ops->label_local( me, 1 );

	release_temp( mop, me, f->m );	

}

void emit_add( struct emitter** mce, struct machine_ops* mop, struct frame* f, loperand d, loperand s, loperand t ){
	emit_bop( REF, mop, f, d, s, t, mop->add, BOP_ADD );
}

void emit_sub( struct emitter** mce, struct machine_ops* mop, struct frame* f, loperand d, loperand s, loperand t ){
	emit_bop( REF, mop, f, d, s, t, mop->sub, BOP_SUB );
}

void emit_mul( struct emitter** mce, struct machine_ops* mop, struct frame* f, loperand d, loperand s, loperand t ){
	emit_bop( REF, mop, f, d, s, t, mop->mul, BOP_MUL );
}

void emit_div( struct emitter** mce, struct machine_ops* mop, struct frame* f, loperand d, loperand s, loperand t ){
	emit_bop( REF, mop, f, d, s, t, mop->sdiv, BOP_DIV );
}

void emit_mod( struct emitter** mce, struct machine_ops* mop, struct frame* f, loperand d, loperand s, loperand t ){
	emit_bop( REF, mop, f, d, s, t, mop->smod, BOP_MOD );
	
	// TODO: convert to floored mod 
	// if ((d > 0 && t < 0) || (d < 0 && t > 0)) d = d+t;
}


