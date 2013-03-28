/*
* (C) Iain Fraser - GPLv3 
*
* Lua platform independent table and upvalue instructions.
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
#include "lstate.h"

#define REF	( *( struct emitter**)mce )

/*
* Upvalue methods. 
*/

static void getupvalptr( struct emitter **mce, struct machine_ops *mop
						, struct frame* f
						, operand uvptr
						, int uvidx ){

	operand closure =  get_frame_closure( f );
	const int uvptr_src = offsetof( struct closure, uvs ) + sizeof( struct UpVal* ) * uvidx;
	const int tval_src = offsetof( struct UpVal, val );

	// deref closure**, then get upval pointer  
	mop->move( REF, f->m, uvptr, closure ); 
	mop->move( REF, f->m, uvptr, OP_TARGETDADDR( uvptr.reg, 0 ) );
	mop->move( REF, f->m, uvptr, OP_TARGETDADDR( uvptr.reg, uvptr_src ) );
	mop->move( REF, f->m, uvptr, OP_TARGETDADDR( uvptr.reg, tval_src ) ); 
}				

static void do_getupval( struct emitter** mce, struct machine_ops* mop, 
						struct frame* f,
						operand dval,
						operand dtype,
						int uvidx ){
	operand uvptr = OP_TARGETREG( acquire_temp( mop, REF, f->m ) );
	getupvalptr( mce, mop, f, uvptr, uvidx );
	mop->move( REF, f->m, dval, OP_TARGETDADDR( uvptr.reg, offsetof( struct TValue, v ) ) );
	mop->move( REF, f->m, dtype, OP_TARGETDADDR( uvptr.reg, offsetof( struct TValue, t ) ) ); 
	release_temp( mop, REF, f->m );
}

void emit_getupval( struct emitter** mce, struct machine_ops* mop
						, struct frame* f
						, loperand dst
						, int uvidx ){
	vreg_operand d = loperand_to_operand( f, dst );
	do_getupval( mce, mop, f, d.value, d.type, uvidx );
}

void emit_setupval( struct emitter** mce, struct machine_ops* mop
						, struct frame* f
						, loperand src
						, int uvidx ){
	operand uvptr = OP_TARGETREG( acquire_temp( mop, REF, f->m ) );
	vreg_operand s = loperand_to_operand( f, src );
	
	getupvalptr( mce, mop, f, uvptr, uvidx );
	mop->move( REF, f->m
			, OP_TARGETDADDR( uvptr.reg, offsetof( struct TValue, v ) )
			, s.value );
	mop->move( REF, f->m
			, OP_TARGETDADDR( uvptr.reg, offsetof( struct TValue, t ) ) 
			, s.type ); 
	release_temp( mop, REF, f->m ); 
}

/*
* Table getter and setter C methods called from Jitter. 
*/

static LUA_PTR ljc_tableget( LUA_PTR tv, lua_Number tt, lua_Number idxt
							, lua_Number idxv
							, LUA_PTR type ){
#define ret( t, v ) 	( *tag = (t), (v) )


	Tag* tag = (Tag*)type;
	struct TValue t = { .t = tt, .v.n = tv };
	struct TValue idx = { .t = idxt, .v.n = idxv };	
	struct TValue* tm = NULL;	// tagged method 	

	if( tag_gentype( &t ) == LUA_TTABLE ){	
		struct TValue v = table_get( (struct table*)tv, idx );
		if( tag( &v ) != LUA_TNIL || 
				!( tm = gettm( &t, TM_INDEX ) )  )
			return ret( v.t, v.v.n );
	} else {
		if( ( tm = gettm( &t, TM_INDEX ) ) == NULL )
			lcurrent_error( LE_TYPE );
	}

	assert( tm );

	struct TValue ret;
	if( !tag_gentype( tm )  == LUA_TFUNCTION )
		lua_call_wrap( current_state(), tm, 2, 1, &t, &idx, &ret );
	else
		return ljc_tableget( tvtonum( tm ), tag( tm ),
					idxt, idxv, type ); 

	return ret( tag( &ret ), tvtonum( &ret ) ); 
}

static void ljc_tableset( LUA_PTR t, lua_Number idxt, lua_Number idxv
							, lua_Number valt
							, lua_Number valv ){
	struct TValue idx = { .t = idxt, .v.n = idxv };	
	struct TValue v = { .t = valt, .v.n = valv };
	table_set( ( struct table*)t, idx, v );
}

/*
* Table methods.
*/


static void do_gettable( struct emitter** mce, struct machine_ops* mop, 
						struct frame* f,
						int dvreg,
						vreg_operand vtable,
						vreg_operand idx ){
	
	vreg_operand dston = vreg_to_operand( f, dvreg, false ); 
	vreg_operand dstoff = vreg_to_operand( f, dvreg, true );
	operand dtptr = OP_TARGETREG( acquire_temp( mop, REF, f->m ) );
	
	address_of( mop, REF, f->m, dtptr, dstoff.type );		
	mop->call_static_cfn( REF, f, (uintptr_t)&ljc_tableget, &dston.value
								, 5 
								, vtable.value
								, vtable.type
								, idx.type 
								, idx.value
								, dtptr );
	release_temp( mop, REF, f->m );
	vreg_type_fill( mop, REF, f, dvreg );
}

static void do_settable( struct emitter** mce, struct machine_ops* mop,
						struct frame* f,
						operand table,
						vreg_operand idx,
						vreg_operand v ){
	mop->call_static_cfn( REF, f, (uintptr_t)&ljc_tableset, NULL
								, 5 
								, table
								, idx.type 
								, idx.value
								, v.type 
								, v.value );
}
					 

void emit_gettable( struct emitter** mce, struct machine_ops* mop
						, struct frame* f 
						, loperand dst
						, loperand table
						, loperand idx ){
	assert( ISLO_LOCAL( dst ) );

	vreg_operand t = loperand_to_operand( f, table );
	do_gettable( mce, mop, f, dst.index, t 
					, loperand_to_operand( f, idx ) );
}

void emit_settable( struct emitter** mce, struct machine_ops* mop
						, struct frame* f 
						, loperand table
						, loperand idx 
						, loperand src ){
	vreg_operand vtable = loperand_to_operand( f, table );
	// TODO: verify its a table
	do_settable( mce, mop, f, vtable.value
				, loperand_to_operand( f, idx )
				, loperand_to_operand( f, src ) );
}

/* get table upvalue (assign to dv/dt) and verify its a table */
static void do_gettableup( struct emitter** mce, struct machine_ops* mop,
						struct frame* f, 
						operand dvalue,
						operand dtype,	
						int uvidx ){
	do_getupval( mce, mop, f, dvalue, dtype, uvidx );
}

void emit_gettableup( struct emitter** mce, struct machine_ops* mop, 
				struct frame* f, loperand dst, int uvidx,
				loperand tidx ){
	vreg_operand d = loperand_to_operand( f, dst );
	do_gettableup( mce, mop, f, d.value, d.type, uvidx );
	do_gettable( mce, mop, f, dst.index, d
				, loperand_to_operand( f, tidx ) );
}

void emit_settableup( struct emitter** mce, struct machine_ops* mop, 
				struct frame* f, int uvidx, loperand tidx,
				loperand value ){
	operand dtype = OP_TARGETREG( acquire_temp( mop, REF, f->m ) );
	operand dval = OP_TARGETREG( acquire_temp( mop, REF, f->m ) );
	do_gettableup( mce, mop, f, dval, dtype, uvidx );
	release_temp( mop, REF, f->m );
	do_settable( mce, mop, f, dval, loperand_to_operand( f, tidx )
					, loperand_to_operand( f, value  ) );
	release_temp( mop, REF, f->m );
}


void emit_self( struct emitter** mce, struct machine_ops* mop
					, struct frame *f
					, loperand dval
					, loperand stable
					, loperand key ) {
	assert( dval.islocal );
	loperand dtable = { .islocal = true, .index = dval.index + 1 };
	vreg_operand vstable = loperand_to_operand( f, stable );

	assign( mop, REF, f->m, loperand_to_operand( f, dtable ), vstable );
	do_gettable( mce, mop, f, dval.index, vstable
					, loperand_to_operand( f, key ) );

}

void emit_newtable( struct emitter** mce, struct machine_ops* mop
							, struct frame* f
							, loperand dst
							, int array, int hash ){
	vreg_operand vres = loperand_to_operand( f, dst );
	mop->call_static_cfn( REF, f, (uintptr_t)&table_create, &vres.value, 2
					, OP_TARGETIMMED( array )
					, OP_TARGETIMMED( hash ) );
	mop->move( REF, f->m, vres.type, OP_TARGETIMMED( ctb( LUA_TTABLE ) ) );
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



