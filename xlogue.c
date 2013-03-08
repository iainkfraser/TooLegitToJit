/*
* (C) Iain Fraser - GPLv3 
*
* Lua function prologue and epilogue code generation. 
*/

#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include "frame.h"
#include "machine.h"
#include "lopcodes.h"
#include "synthetic.h"
#include "lua.h"
#include "stack.h"
#include "jitfunc.h"

static int stack_frame_size( int nr_locals ){
	int k = 4 * 4;	// ra, closure, return position, nr results 
	return nr_locals * 8 + k;
}

/* Grab non temps first then temps */
static void prefer_nontemp_acquire_reg( struct machine_ops* mop, struct emitter* e, struct machine* m, 
					int n, operand reg[n] ){
	// must fit into registers only
	assert( n <= m->nr_reg );

	const int nr_temps = m->nr_temp_regs;
	const int nr_nontemps = m->nr_reg - nr_temps;

	for( int i = 0; i < n; i++ ){
		if( i < nr_nontemps )
			reg[i] = OP_TARGETREG( m->reg[ nr_temps + i ] );
		else
			reg[i] = OP_TARGETREG( acquire_temp( mop, e, m ) );	// ( i - nr_nontemps );	
	}
}

static void prefer_nontemp_release_reg( struct machine_ops* mop, struct emitter* e, struct machine* m, int n ){
	const int nr_temps = m->nr_temp_regs;
	const int nr_nontemps = m->nr_reg - nr_temps;

	for( int i = nr_nontemps; i < n; i++ ){
		release_temp( mop, e, m );
	}
}


// in order of nontemporary register assignment priority 
enum REGARGS { RA_NR_ARGS, RA_BASE, RA_COUNT };
//enum { RA_DST, RA_SRC, RA_EXIST, RA_EXPECT, RA_SIZE };
enum { RA_EXIST, RA_SRC, RA_DST, RA_EXPECT, RA_SIZE };

static void precall( struct machine_ops* mop, struct emitter* e, struct frame* f, int vregbase, int narg, int nret ){
	// new frame assumes no temporaries have been used yet 
	assert( temps_accessed( f->m ) == 0 );
	assert( RA_COUNT + 1 <= f->m->nr_reg );		// regargs are passed by register NOT stack

	vreg_operand clive = vreg_to_operand( f, vregbase, false );
 	vreg_operand cstack = vreg_to_operand( f, vregbase, true );
	assert( cstack.value.tag == OT_DIRECTADDR );

	// TODO: verify its a closure using clive 
	
	// get arg passing registers
	operand rargs[ RA_COUNT ];
	prefer_nontemp_acquire_reg( mop, e, f->m, RA_COUNT, rargs );

	// calculate number of args
	if( narg > 0 )
		mop->move( e, f->m, rargs[ RA_NR_ARGS ], OP_TARGETIMMED( narg - 1 ) );
	else{
		// calculate total by subtracting basereg address from stack.

		// 2 becuase 8 for (ebp,closure) another 8 for the function being called ( rem actual args = args - 1 )
		mop->add( e, f->m, rargs[ RA_NR_ARGS ], OP_TARGETREG( f->m->fp ), OP_TARGETIMMED( -8 * ( 2 + vregbase ) ) );
		mop->sub( e, f->m, rargs[ RA_NR_ARGS ], rargs[ RA_NR_ARGS ], OP_TARGETREG( f->m->sp ) );	
		mop->udiv( e, f->m, rargs[ RA_NR_ARGS ], rargs[ RA_NR_ARGS ], OP_TARGETIMMED( 8 ) );
	}

	// calcualte base address	
	mop->add( e, f->m, rargs[ RA_BASE ], OP_TARGETREG( cstack.value.base ), OP_TARGETIMMED( cstack.value.offset ) );
	
	// call function without spilling any temps
	bool prior = disable_spill( f->m );
	mop->call( e, f->m, clive.value );
	restore_spill( f->m, prior );

	// release temps used in call
	prefer_nontemp_release_reg( mop, e, f->m, RA_COUNT );
}

static void postcall( struct machine_ops* mop, struct emitter* e, struct frame* f, int vregbase, int narg, int nret ){
 	vreg_operand basestack = vreg_to_operand( f, vregbase, true );
	
	operand rargs[ RA_SIZE ];
	prefer_nontemp_acquire_reg( mop, e, f->m, RA_SIZE, rargs );

	// max stack clobber 
	const int maxstack = 3;	// prior frame has buffer of pushed return addr, frame pointer and closure

	// set dst and expect
	mop->add( e, f->m, rargs[ RA_DST ], OP_TARGETREG( basestack.value.base ), OP_TARGETIMMED( basestack.value.offset ) );

	if( nret == 0 ){
		// set exist 	
		mop->move( e, f->m, rargs[ RA_EXPECT ], rargs[ RA_EXIST ] );
		
		// update stack position 
		mop->mul( e, f->m, rargs[ RA_NR_ARGS ], rargs[ RA_NR_ARGS ], OP_TARGETIMMED( 8 ) );	// in word units
		mop->add( e, f->m, rargs[ RA_NR_ARGS ], rargs[ RA_NR_ARGS ], OP_TARGETIMMED( 8 + 8 * vregbase ) );
		mop->sub( e, f->m, OP_TARGETREG( f->m->sp ), OP_TARGETREG( f->m->fp ), rargs[ RA_NR_ARGS ] );

		mop->move( e, f->m, rargs[ RA_EXIST ], rargs[ RA_EXPECT ]  );	
	} else {
		mop->move( e, f->m, rargs[ RA_EXPECT ], OP_TARGETIMMED( nret - 1 ) );
	}


	jfunc_call( mop, e, f->m, JF_ARG_RES_CPY, 0, maxstack, 4, rargs[ RA_SRC ], rargs[ RA_DST ], 
						rargs[ RA_EXPECT ], rargs[ RA_EXIST ] );
	prefer_nontemp_release_reg( mop, e, f->m, RA_SIZE );
}

void do_call( struct machine_ops* mop, struct emitter* e, struct frame* f, int vregbase, int narg, int nret ){
	precall( mop, e, f, vregbase, narg, nret );
	postcall( mop, e, f, vregbase, narg, nret );
}

/*
* Load the following and call epilogue:
* 	r[ RA_NR_ARGS ] = number of results
*	r[ RA_BASE ] = start address of results 
*/
void do_ret( struct machine_ops* mop, struct emitter* e, struct frame* f, int vregbase, int nret ){
	assert( RA_COUNT <= f->m->nr_reg );	// regargs are passed by register NOT stack
	vreg_operand basestack = vreg_to_operand( f, vregbase, true );

	/*
	* if nret == 0 then prev instruction was call and calls save vregs on stack. Therefore
	* only when nret > 0 do we need to save live regs onto stack. Do it first so it can
	* use as many temps as it wants before we reserve them for return procedure. 
	*/
	if( nret > 0 ) 
#if 1
		save_frame_limit( mop, e, f, vregbase, nret - 1 );
#else
		// TODO: the above is from vregbase - so need to think about this one
		jfunc_call( mop, e, f->m, JF_STORE_LOCALS, jf_storelocal_offset( f->m, nret - 1 ), JFUNC_UNLIMITED_STACK, 0 );
#endif

	operand rargs[ RA_COUNT ];
	prefer_nontemp_acquire_reg( mop, e, f->m, RA_COUNT, rargs );

	if( nret > 0 ) {
		mop->move( e, f->m, rargs[ RA_NR_ARGS ], OP_TARGETIMMED( nret - 1 ) );
	} else {
		; 	// TODO: calculate total results 
	}

	mop->add( e, f->m, rargs[ RA_BASE ], OP_TARGETREG( basestack.value.base ), OP_TARGETIMMED( basestack.value.offset ) );
 	mop->b( e, f->m, LBL_EC( f->epi ) ); 
}

void prologue( struct machine_ops* mop, struct emitter* e, struct frame* f ){
	// new frame assumes no temporaries have been used yet 
	assert( temps_accessed( f->m ) == 0 );

	const operand sp = OP_TARGETREG( f->m->sp );
	const operand fp = OP_TARGETREG( f->m->fp );
	const int nparams = f->nr_params;
	
	operand rargs[ RA_SIZE ];
	prefer_nontemp_acquire_reg( mop, e, f->m, RA_SIZE, rargs );

	// push old frame pointer, closure addr / result start addr, expected nr or results 
	pushn( mop, e, f->m, 2, fp, rargs[ RA_SRC ] ); 
	
	// set ebp and update stack
	mop->add( e, f->m, fp, sp, OP_TARGETIMMED( 4 ) );	// point to ebp so add 4 
	mop->add( e, f->m, sp, sp, OP_TARGETIMMED( -( 8 * f->nr_locals ) ) );

	if( nparams ) {
		const vreg_operand basestack = vreg_to_operand( f, 0, true );		// destination is first local
		const int maxstack = JFUNC_UNLIMITED_STACK; 

		// set src ( always start after closure see Lua VM for reason )
		mop->add( e, f->m, rargs[ RA_SRC ], rargs[ RA_SRC ], OP_TARGETIMMED( -8 ) );

		// set dst and expect
		mop->add( e, f->m, rargs[ RA_DST ], OP_TARGETREG( basestack.value.base ), OP_TARGETIMMED( basestack.value.offset ) );
		mop->move( e, f->m, rargs[ RA_EXPECT ], OP_TARGETIMMED( nparams ) );
		
		jfunc_call( mop, e, f->m, JF_ARG_RES_CPY, 0, maxstack, 4, rargs[ RA_SRC ], rargs[ RA_DST ], 
							rargs[ RA_EXPECT ], rargs[ RA_EXIST ] );
		prefer_nontemp_release_reg( mop, e, f->m, RA_SIZE );

#if 0	 	
		load_frame_limit( mop, e, f, 0, nparams );	// load locals living in registers 
#else
		jfunc_call( mop, e, f->m, JF_LOAD_LOCALS, jf_loadlocal_offset( f->m, nparams ), maxstack, 0 );
#endif
	}
}

void epilogue( struct machine_ops* mop, struct emitter* e, struct frame* f ){
	const operand sp = OP_TARGETREG( f->m->sp );
	const operand fp = OP_TARGETREG( f->m->fp );

	// reset stack 
	mop->move( e, f->m, sp, fp );
	pop( mop, e, f->m, fp );
	mop->ret( e, f->m );
}

/*
* Generate JFunction for copying arguments and results. Spills must be minimized
* to stop clobbering the previous frames locals.
*/


static void do_copying( struct machine_ops* mop, struct emitter* e, struct machine* m, 
		operand iter, operand limit, operand dst, operand src ){

	// start loop 
	e->ops->label_local( e, 0 );
	mop->beq( e, m, iter, limit, LBL_NEXT( 0 ) ); 	// TODO: bgt is equiv to beq so swap? 

	// copy 
	mop->move( e, m, OP_TARGETDADDR( dst.reg, 0 ), OP_TARGETDADDR( src.reg, 0 ) );
	mop->move( e, m, OP_TARGETDADDR( dst.reg, -4 ), OP_TARGETDADDR( src.reg, -4 ) );
	
	// update pointers 
	mop->add( e, m, dst, dst, OP_TARGETIMMED( -8 ) );	
	mop->add( e, m, src, src, OP_TARGETIMMED( -8 ) ); 
	
	// update iterator
	mop->add( e, m, iter, iter, OP_TARGETIMMED( 1 ) );
	
	mop->b( e, m, LBL_PREV( 0 ) );
	e->ops->label_local( e, 0 );
}

static void do_niling( struct machine_ops* mop, struct emitter* e, struct machine* m, 
		operand iter, operand limit, operand dst, operand src ){

	// start loop 
	e->ops->label_local( e, 0 );
	mop->beq( e, m, iter, limit, LBL_NEXT( 0 ) ); 	// TODO: bgt is equiv to beq so swap? 

	// copy 
	mop->move( e, m, OP_TARGETDADDR( dst.reg, 0 ), OP_TARGETDADDR( src.reg, 0 ) );
	mop->move( e, m, OP_TARGETDADDR( dst.reg, -4 ), OP_TARGETDADDR( src.reg, -4 ) );
	
	// update pointers 
	mop->add( e, m, dst, dst, OP_TARGETIMMED( -8 ) );	
	mop->add( e, m, src, src, OP_TARGETIMMED( -8 ) ); 
	
	// update iterator
	mop->add( e, m, iter, iter, OP_TARGETIMMED( 1 ) );
	
	mop->b( e, m, LBL_PREV( 0 ) );
	e->ops->label_local( e, 0 );
}

static void do_nilling( struct machine_ops* mop, struct emitter* e, struct machine* m, 
		operand iter, operand limit, operand dst ){

	// start loop 
	e->ops->label_local( e, 0 );
	mop->beq( e, m, iter, limit, LBL_NEXT( 0 ) ); 	// TODO: bgt is equiv to beq so swap? 

	// copy 
	mop->move( e, m, OP_TARGETDADDR( dst.reg, -4 ), OP_TARGETIMMED( LUA_TNIL ) );
	
	// update pointers 
	mop->add( e, m, dst, dst, OP_TARGETIMMED( -8 ) );	
	
	// update iterator
	mop->add( e, m, iter, iter, OP_TARGETIMMED( 1 ) );
	
	mop->b( e, m, LBL_PREV( 0 ) );
	e->ops->label_local( e, 0 );
}

void jinit_cpy_arg_res( struct JFunc* jf, struct machine_ops* mop, struct emitter* e, struct machine* m ){
	operand rargs[ RA_SIZE ];
	prefer_nontemp_acquire_reg( mop, e, m, RA_SIZE, rargs );

	syn_min( mop, e, m, rargs[ RA_EXIST ], rargs[ RA_EXIST ], rargs[ RA_EXPECT ] );
	
	// init iterator 
	operand iter = OP_TARGETREG( acquire_temp( mop, e, m ) );			
	mop->move( e, m, iter, OP_TARGETIMMED( 0 ) );
	
	do_copying( mop, e, m, iter, rargs[ RA_EXIST ], rargs[ RA_DST ], rargs[ RA_SRC ] );
	
	/* 
	* TODO: Below logic is incorrect the first is most likely to get spilled. So change enum order. BUT 
	* then you have to think about prefer saved reg and the order there.
	* TODO: add error prefer_nontemps to error when not all live simultenously 
	*
	* RA_EXPECT is last register therefore its the most likely to be spilled. So to stop repeat
	* spill/unspill move it to exist.
	*/
	mop->move( e, m, rargs[ RA_SIZE ], rargs[ RA_EXIST ] );
	do_nilling( mop, e, m, iter, rargs[ RA_EXIST ], rargs[ RA_DST ] );

	release_temp( mop, e, m );	
	prefer_nontemp_release_reg( mop, e, m, RA_SIZE );

	mop->ret( e, m );
}

