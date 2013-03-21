/*
* (C) Iain Fraser - GPLv3 
*
* Lua function prologue and epilogue code generation. The code is a bit
* of a mess because I moved the naive inline version to the function
* calling version and didnt delete the original but instead commneted it
* out using the preprocessor.  
*/

#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include "frame.h"
#include "machine.h"
#include "lopcodes.h"
#include "synthetic.h"
#include "stack.h"
#include "jitfunc.h"
#include "func.h"
#include "lstate.h"
#include "lobject.h"

static LUA_PTR ljc_invokec( LUA_PTR base, size_t n, lua_Number* nres );

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

	// disable spill
	disable_spill( m );
}

static void prefer_nontemp_release_reg( struct machine_ops* mop, struct emitter* e, struct machine* m, int n ){
	const int nr_temps = m->nr_temp_regs;
	const int nr_nontemps = m->nr_reg - nr_temps;

	for( int i = nr_nontemps; i < n; i++ ){
		release_temp( mop, e, m );
	}

	enable_spill( m );
}


// in order of nontemporary register assignment priority 
enum REGARGS { RA_NR_ARGS, RA_BASE, RA_COUNT };
//enum { RA_DST, RA_SRC, RA_EXIST, RA_EXPECT, RA_SIZE };
enum { RA_EXIST, RA_SRC, RA_DST, RA_EXPECT, RA_SIZE };

static void precall( struct machine_ops* mop, struct emitter* e, struct frame* f,
				 int vregbase, int narg, int nret ){
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
		/* 
		* Calculate total by subtracting basereg address from stack.
		* 2 becuase 8 for (ebp,closure) another 8 for the function 
		* being called ( rem actual args = args - 1 )
		*/
		mop->add( e, f->m, rargs[ RA_NR_ARGS ], OP_TARGETREG( f->m->fp )
					, OP_TARGETIMMED( -8 * ( 2 + vregbase ) ) );

		mop->sub( e, f->m, rargs[ RA_NR_ARGS ], rargs[ RA_NR_ARGS ]
					, OP_TARGETREG( f->m->sp ) );	

		mop->udiv( e, f->m, rargs[ RA_NR_ARGS ], rargs[ RA_NR_ARGS ]
					, OP_TARGETIMMED( 8 ) );
	}

	// calcualte base address	
	address_of( mop, e, f->m, rargs[ RA_BASE ], cstack.value );
	
	// call function without spilling any temps
	jfunc_call( mop, e, f->m, JF_PROLOGUE, 0, JFUNC_UNLIMITED_STACK, 2
					, rargs[ RA_EXIST ], rargs[ RA_SRC ] );

	// release temps used in call
	prefer_nontemp_release_reg( mop, e, f->m, RA_COUNT );
}

static void postcall( struct machine_ops* mop, struct emitter* e, struct frame* f, int vregbase, int narg, int nret ){
 	vreg_operand basestack = vreg_to_operand( f, vregbase, true );
	
	operand rargs[ RA_SIZE ];
	prefer_nontemp_acquire_reg( mop, e, f->m, RA_SIZE, rargs );

	// max stack clobber 
	const int maxstack = 3;	// prior frame has buffer of pushed return addr, frame pointer and closure
#define USE_JFUNC_FOR_VARRES

	// set dst and expect
#if 0 
	mop->add( e, f->m, rargs[ RA_DST ], OP_TARGETREG( basestack.value.base ), OP_TARGETIMMED( basestack.value.offset ) );
#endif

	if( nret == 0 ){
#ifdef USE_JFUNC_FOR_VARRES 
		jfunc_call( mop, e, f->m, JF_VARRES_POSTCALL, 0, maxstack, 4, rargs[ RA_SRC ], rargs[ RA_DST ], 
						rargs[ RA_EXPECT ], rargs[ RA_EXIST ] );
#else
		mop->move( e, f->m, rargs[ RA_EXPECT ], rargs[ RA_EXIST ] );
		
		// update stack position 
		mop->mul( e, f->m, rargs[ RA_NR_ARGS ], rargs[ RA_NR_ARGS ], OP_TARGETIMMED( 8 ) );	// in word units
		mop->add( e, f->m, rargs[ RA_NR_ARGS ], rargs[ RA_NR_ARGS ], OP_TARGETIMMED( 8 + 8 * vregbase ) );
		mop->sub( e, f->m, OP_TARGETREG( f->m->sp ), OP_TARGETREG( f->m->fp ), rargs[ RA_NR_ARGS ] );

		mop->move( e, f->m, rargs[ RA_EXIST ], rargs[ RA_EXPECT ]  );	
#endif
	} else {
		mop->move( e, f->m, rargs[ RA_EXPECT ], OP_TARGETIMMED( nret - 1 ) );
#ifdef USE_JFUNC_FOR_VARRES 
		jfunc_call( mop, e, f->m, JF_ARG_RES_CPY, 0, maxstack, 4, rargs[ RA_SRC ], rargs[ RA_DST ], 
						rargs[ RA_EXPECT ], rargs[ RA_EXIST ] );
#endif
	}

#ifndef USE_JFUNC_FOR_VARRES
		jfunc_call( mop, e, f->m, JF_ARG_RES_CPY, 0, maxstack, 4, rargs[ RA_SRC ], rargs[ RA_DST ], 
						rargs[ RA_EXPECT ], rargs[ RA_EXIST ] );
#endif

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

	mop->add( e, f->m, rargs[ RA_BASE ], OP_TARGETREG( basestack.value.base ), OP_TARGETIMMED( basestack.value.offset ) );

	if( nret > 0 ) {
		mop->move( e, f->m, rargs[ RA_NR_ARGS ], OP_TARGETIMMED( nret - 1 ) );
	} else {
		mop->sub( e, f->m, rargs[ RA_NR_ARGS ], rargs[ RA_BASE ], OP_TARGETREG( f->m->sp ) );	
		mop->udiv( e, f->m, rargs[ RA_NR_ARGS ], rargs[ RA_NR_ARGS ], OP_TARGETIMMED( 8 ) );
	}

	mop->b( e, f->m, LBL_ABS( OP_TARGETIMMED( (uintptr_t)jfunc_addr( e, JF_EPILOGUE ) ) ) ); 
}

void prologue( struct machine_ops* mop, struct emitter* e, struct frame* f ){
#if 1
	const operand sp = OP_TARGETREG( f->m->sp );
	const operand fp = OP_TARGETREG( f->m->fp );
	const int nparams = f->nr_params;

	operand rargs[ RA_SIZE ];
	prefer_nontemp_acquire_reg( mop, e, f->m, RA_SIZE, rargs );
	
	mop->add( e, f->m, sp, sp, OP_TARGETIMMED( -( 8 * f->nr_locals ) ) );
	if( nparams ){
		// set nparams
		mop->move( e, f->m, rargs[ RA_EXPECT ], OP_TARGETIMMED( nparams ) );
		
		// do argument cpy
		jfunc_call( mop, e, f->m, JF_ARG_RES_CPY, 0, JFUNC_UNLIMITED_STACK, 4, rargs[ RA_SRC ], rargs[ RA_DST ], 
							rargs[ RA_EXPECT ], rargs[ RA_EXIST ] );
		
		// do call
		jfunc_call( mop, e, f->m, JF_LOAD_LOCALS, jf_loadlocal_offset( f->m, nparams ), JFUNC_UNLIMITED_STACK, 0 );
	}
	
	prefer_nontemp_release_reg( mop, e, f->m, RA_SIZE );
#else
	// new frame assumes no temporaries have been used yet 
	assert( temps_accessed( f->m ) == 0 );

	const operand sp = OP_TARGETREG( f->m->sp );
	const operand fp = OP_TARGETREG( f->m->fp );
	const int nparams = f->nr_params;
	
	operand rargs[ RA_SIZE ];
	prefer_nontemp_acquire_reg( mop, e, f->m, RA_SIZE, rargs );

	// push old frame pointer, closure addr / result start addr, expected nr or results 
	if( f->m->is_ra )
		pushn( mop, e, f->m, 3, OP_TARGETREG( f->m->ra), fp, rargs[ RA_SRC ] ); 
	else
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
#endif
}

void epilogue( struct machine_ops* mop, struct emitter* e, struct frame* f ){
#if 0	// epilogue is now JFunction - so shared amongst all functions 
	const operand sp = OP_TARGETREG( f->m->sp );
	const operand fp = OP_TARGETREG( f->m->fp );

	// reset stack 
	mop->move( e, f->m, sp, fp );
	pop( mop, e, f->m, fp );
	mop->ret( e, f->m );
#endif
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


void jinit_epi( struct JFunc* jf, struct machine_ops* mop, struct emitter* e, struct machine* m ){
	// phoney frame 
	struct frame F = { .m = m, .nr_locals = 0, .nr_params = 0 };
	struct frame *f = &F;

	const operand sp = OP_TARGETREG( m->sp );
	const operand fp = OP_TARGETREG( m->fp );

	// migrate closed upvalues 
	mop->call_static_cfn( e, f, (uintptr_t)&closure_close, NULL, 1, get_frame_closure( f ) );  
	
	operand rargs[ RA_SIZE ];
	prefer_nontemp_acquire_reg( mop, e, f->m, RA_SIZE, rargs );

	// reset stack 
	mop->add( e, m, sp, fp, OP_TARGETIMMED( -4 ) );
	if( m->is_ra ) 
		popn( mop, e, m , 3, rargs[ RA_DST ], fp, OP_TARGETREG( m->ra ) );
	else
		popn( mop, e, m, 2, rargs[ RA_DST ], fp ); 

	mop->ret( e, m );
	
	prefer_nontemp_release_reg( mop, e, f->m, RA_SIZE );
}

/* 
* If Lua closure do the majority ( function independent ) of the prologue. 
* That is: store the frame section, update src and dst pointers and call 
* the memcpy.
*
* The function specific code needs to set the number of params, update the 
* stack ( requires # of locals ) and then unspill params.  
*/
static void lcl_pro( struct machine_ops* mop, struct emitter* e, 
			struct machine* m ){
	// phoney frame 
	struct frame F = { .m = m, .nr_locals = 1, .nr_params = 0 };
	struct frame *f = &F;

	const int maxstack = JFUNC_UNLIMITED_STACK; 
	const operand sp = OP_TARGETREG( f->m->sp );
	const operand fp = OP_TARGETREG( f->m->fp );

	// destination is first local
	const vreg_operand basestack = vreg_to_operand( f, 0, true );		
	
	operand rargs[ RA_SIZE ];
	prefer_nontemp_acquire_reg( mop, e, f->m, RA_SIZE, rargs );

	// push old frame pointer, closure addr / result start addr, expected nr or results 
	if( f->m->is_ra )
		pushn( mop, e, f->m, 3, OP_TARGETREG( f->m->ra), fp, rargs[ RA_SRC ] ); 
	else
		pushn( mop, e, f->m, 2, fp, rargs[ RA_SRC ] ); 
	
	// set ebp and update stack
	mop->add( e, f->m, fp, sp, OP_TARGETIMMED( 4 ) );	// point to ebp so add 4 

	/*
	* Get start instruction from closure by: rargs[ RA_SRC ]->p->code_start. Use rargs[ RA_EXPECT ] which is
	* not set precall. TODO: alias the enum.  
	*/	
	mop->move( e, f->m, rargs[ RA_EXPECT ], OP_TARGETDADDR( rargs[ RA_SRC ].reg, 0 ) ); 
	mop->move( e, f->m, rargs[ RA_EXPECT ], OP_TARGETDADDR( rargs[ RA_EXPECT ].reg, offsetof( struct closure, p ) ) );
	mop->move( e, f->m, rargs[ RA_EXPECT ], OP_TARGETDADDR( rargs[ RA_EXPECT ].reg, offsetof( struct proto, code_start ) ) );

	// set src ( always start after closure see Lua VM for reason )
	mop->add( e, f->m, rargs[ RA_SRC ], rargs[ RA_SRC ], OP_TARGETIMMED( -8 ) );
	mop->add( e, f->m, rargs[ RA_DST ], OP_TARGETREG( basestack.value.base ), OP_TARGETIMMED( basestack.value.offset ) );

	/*
	* Call the actual function, which is the closure. On RISC this will clobber
	* temp hopefully this isn't a live reg or we will get exception. On CISC
	* there is probably indirect direct address jmp instruction ( x86 does 0 ). 
	*/
#if 0
	mop->b( e, f->m, LBL_ABS( OP_TARGETDADDR( rargs[ RA_SRC ].reg, 8 ) ) );
#else
	mop->b( e, f->m, LBL_ABS( rargs[ RA_EXPECT ] ) );
#endif

	prefer_nontemp_release_reg( mop, e, f->m, RA_SIZE );
}

/*
* Call the C function invoker. 
*/
static void lcf_pro( struct machine_ops *mop, struct emitter *e, struct machine *m ){
	// phoney frame 
	struct frame F = { .m = m, .nr_locals = 0, .nr_params = 0 };
	struct frame *f = &F;

	const operand ra = OP_TARGETREG( f->m->ra );
	const operand sp = OP_TARGETREG( f->m->sp );

	operand rargs[ RA_SIZE ];
	prefer_nontemp_acquire_reg( mop, e, m, RA_SIZE, rargs );
	prefer_nontemp_release_reg( mop, e, m, RA_SIZE );

	// need to store return address 
	if( f->m->is_ra )
		pushn( mop, e, f->m, 2, ra, rargs[ RA_BASE ] ); 
	else
		pushn( mop, e, f->m, 1, rargs[ RA_BASE ] );

	// need pointer to get number of results, use RA_EXPECT as not used.
	mop->add( e, f->m, sp, sp, OP_TARGETIMMED( -4 ) ); 
	mop->move( e,f->m, rargs[ RA_EXPECT ], sp ); 

	/*
	* Reuse RA_BASE is uses:
	*	1) It is the C closure on entry
	*	2) The register will contain result src ptr on exit
	*/	
	mop->call_static_cfn( e, f, (uintptr_t)&ljc_invokec, &rargs[ RA_BASE ] 
							, 3 
							, rargs[ RA_BASE ]
							, rargs[ RA_NR_ARGS ]
							, rargs[ RA_EXPECT ] );

	// get number of results.
	mop->move( e, f->m, rargs[RA_NR_ARGS], OP_TARGETDADDR( sp.reg, 0 ) );
	mop->add( e, f->m, sp, sp, OP_TARGETIMMED( 4 ) ); 
		

	if( f->m->is_ra )
		popn( mop, e, f->m, 2, rargs[ RA_DST ], ra ); 
	else
		popn( mop, e, f->m, 1, rargs[ RA_DST ] );

	mop->ret( e, m );
}

/*
* Determine the type of function. If light C function then call the C invoker
* function.
*/
void jinit_pro( struct JFunc* jf, struct machine_ops* mop, struct emitter* e,
				 struct machine* m ){

	operand rargs[ RA_SIZE ];
	prefer_nontemp_acquire_reg( mop, e, m, RA_SIZE, rargs );

	// expect has not been used yet, so use it store type of function
	operand tag = OP_TARGETDADDR( rargs[ RA_BASE ].reg
				, vreg_type_offset( 0 ) );
	mop->move( e, m, rargs[ RA_EXPECT ], tag );
			

	mop->beq( e, m, rargs[ RA_EXPECT ], OP_TARGETIMMED( ctb( LUA_TLCL ) )
			, LBL_NEXT( 0 ) ); 
	mop->beq( e, m, rargs[ RA_EXPECT ], OP_TARGETIMMED( LUA_TLCF )
			, LBL_NEXT( 1 ) ); 
	// TODO: C closure
	// TODO: runtime Lua error

	prefer_nontemp_release_reg( mop, e, m, RA_SIZE );

	e->ops->label_local( e, 0 );
	lcl_pro( mop, e, m );
	e->ops->label_local( e, 1 );
	lcf_pro( mop, e, m );	
}

/*
* The number of results is not know before hand. Need to update stack for future
* calls.
*/
void jinit_vresult_postcall( struct JFunc* jf, struct machine_ops* mop, struct emitter* e, struct machine* m ){
	// phoney frame 
	struct frame F = { .m = m, .nr_locals = 1, .nr_params = 0 };
	struct frame *f = &F;

	operand rargs[ RA_SIZE ];
	prefer_nontemp_acquire_reg( mop, e, f->m, RA_SIZE, rargs );

	// max stack clobber 
	const int maxstack = 3;	// prior frame has buffer of pushed return addr, frame pointer and closure

	// consume as many results as available 
	mop->move( e, f->m, rargs[ RA_EXPECT ], rargs[ RA_EXIST ] );
	
	// if register based remember return address 
	if( f->m->is_ra )
		pushn( mop, e, f->m, 1, OP_TARGETREG( f->m->ra) );	// not safe cause of stack

	// copy args across 
	jfunc_call( mop, e, f->m, JF_ARG_RES_CPY, 0, maxstack, 4
						, rargs[ RA_SRC ]
						, rargs[ RA_DST ]
						, rargs[ RA_EXPECT ]
						, rargs[ RA_EXIST ] );

	if( f->m->is_ra )
		popn( mop, e, f->m, 1, OP_TARGETREG( f->m->ra) );

	
	/*
	* this depends heavily on copy arg implementation, it assumes ptrs will point to 
	* the top of the stack after copying i.e. the last result copied.
	*/
	mop->add( e, m, OP_TARGETREG( f->m->sp ), rargs[ RA_DST ], OP_TARGETIMMED( 0 ) ); 

	prefer_nontemp_release_reg( mop, e, f->m, RA_SIZE );

	// return 
	mop->ret( e, m );

	
}

/*
* Bootstrap JIT code. The prototype for this function is:
*	int bootstrap( int nr_args, struct Value* closure )
* should return the number of results, and expects . 
*/
void jinit_bootstrap( struct JFunc* jf, struct machine_ops* mop 
						, struct emitter* e
						, struct machine* m ){
	operand rargs[ RA_SIZE ];
	prefer_nontemp_acquire_reg( mop, e, m, RA_SIZE, rargs );

	if( m->is_ra )
		push( mop, e, m, OP_TARGETREG( m->ra ) ); 

	// move to expected args 
	mop->move( e, m, rargs[ RA_EXIST ], mop->carg( m, 0 ) );
	mop->move( e, m, rargs[ RA_SRC ], mop->carg( m, 1 ) );

	// call prologue 
	jfunc_call( mop, e, m, JF_PROLOGUE, 0, JFUNC_UNLIMITED_STACK, 2
					, rargs[ RA_EXIST ], rargs[ RA_SRC ] );

	// set the return src to closure ( which is now dst )
	mop->move( e, m, OP_TARGETDADDR( rargs[ RA_DST ].reg, 0 )
				, rargs[ RA_SRC ] );
	mop->move( e, m, mop->cret( m ), rargs[ RA_EXIST ] );


	prefer_nontemp_release_reg( mop, e, m, RA_SIZE );

	if( m->is_ra )
		pop( mop, e, m, OP_TARGETREG( m->ra ) ); 

	mop->ret( e, m );
}


/*
* Call from Jitted code. The base pointer initally points to the
* C function to be called and ptr + vreg is start of arguments. On
* return the results are stored starting at base.
*/

static LUA_PTR ljc_invokec( LUA_PTR base, size_t n, lua_Number* nres ){
	struct TValue* cfn = (struct TValue*)( base + vreg_type_offset( 0 ) ); 
	struct TValue* argv = cfn - 1;
	struct TValue* res = cfn;
	lua_State* L = current_state();

	// setup new call stack
	struct TValue callstack[ LUA_MINSTACK ];
	struct TValue *oldstack = L->stack;
	struct TValue *oldtop = L->top;
	initstack( L, callstack, LUA_MINSTACK ); 
	
	// copy args to vstack
	for( int i = 1; i <= n; i++, argv-- )
		lua_safepush( L, *argv );	

	// only set nres after call function because  *maybe* ptr are equal
	*nres = (lua_Number)cfn->v.f( L );

	/*
	* Get pointer to first result value. Will be copied later on
	* by postcall functions. Presuming that C won't clobber this
	* callframe ( i.e. the callstack array ), but why would it?
	*/
	struct TValue* ret = index2addr( L, -*nres );

	// restore all stack frame
	L->stack = oldstack;
	L->top = oldtop; 

	return (LUA_PTR)&ret->v;
} 
