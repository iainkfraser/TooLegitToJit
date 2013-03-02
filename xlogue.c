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

static int stack_frame_size( int nr_locals ){
	int k = 4 * 4;	// ra, closure, return position, nr results 
	return nr_locals * 8 + k;
}

/* Grab non temps first then temps */
static void prefer_nontemp_acquire_reg( struct machine_ops* mop, struct emitter* e, struct machine* m, 
					int n, operand reg[n] ){
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


static void caller_prologue( struct machine_ops* mop, struct emitter* e, struct frame* f, int vregbase, int narg, int nret ){
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
		mop->div( e, f->m, rargs[ RA_NR_ARGS ], rargs[ RA_NR_ARGS ], OP_TARGETIMMED( 8 ) );
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

static void caller_epilogue( struct machine_ops* mop, struct emitter* e, struct frame* f, int vregbase, int narg, int nret ){
 	vreg_operand basestack = vreg_to_operand( f, vregbase, true );
	
	operand rargs[ RA_COUNT ];
	prefer_nontemp_acquire_reg( mop, e, f->m, RA_COUNT, rargs );
	
	// get destination pointer 
	operand dst = OP_TARGETREG( acquire_temp( mop, e, f->m ) );
	mop->add( e, f->m, dst, OP_TARGETREG( basestack.value.base ), OP_TARGETIMMED( basestack.value.offset ) );		
	
	if( nret > 0 ){
		nret = nret - 1;

		syn_min( mop, e, f->m, rargs[ RA_NR_ARGS], rargs[ RA_NR_ARGS ], OP_TARGETIMMED( nret ) );
		mop->mul( e, f->m, rargs[ RA_NR_ARGS ], rargs[ RA_NR_ARGS ], OP_TARGETIMMED( 2 ) );
		syn_memcpyw( mop, e, f->m, dst, rargs[ RA_BASE ], rargs[ RA_NR_ARGS ] );
		
		// update pointers to after memcpy location
		mop->mul( e, f->m, rargs[ RA_NR_ARGS ], rargs[ RA_NR_ARGS ], OP_TARGETIMMED( 2 ) );
		mop->add( e, f->m, rargs[ RA_BASE ], rargs[ RA_BASE ], rargs[ RA_NR_ARGS ] );
		mop->add( e, f->m, dst, dst, rargs[ RA_NR_ARGS] );	
		mop->div( e, f->m, rargs[ RA_NR_ARGS ], rargs[ RA_NR_ARGS ], OP_TARGETIMMED( 2 ) );
		
		// memset nil ( don't worry about writing value, its ignored when nil )
		mop->sub( e, f->m, rargs[ RA_NR_ARGS ], rargs[ RA_NR_ARGS ], OP_TARGETIMMED( nret * 2 ) );	
		syn_memsetw( mop, e, f->m, dst, OP_TARGETIMMED( LUA_TNIL ), rargs[ RA_NR_ARGS ] );
	} else {
		assert( nret == 0 );
		const operand sp = OP_TARGETREG( f->m->sp );
		const operand fp = OP_TARGETREG( f->m->fp );

		// copy ALL the returned results 
		mop->mul( e, f->m, rargs[ RA_NR_ARGS ], rargs[ RA_NR_ARGS ], OP_TARGETIMMED( 2 ) );	// type and value
		syn_memcpyw( mop, e, f->m, dst, rargs[ RA_BASE ], rargs[ RA_NR_ARGS ] );
	
		// change stack pointer to reflect returned results 	
		mop->mul( e, f->m, rargs[ RA_NR_ARGS ], rargs[ RA_NR_ARGS ], OP_TARGETIMMED( 4 ) );	// in word units
		mop->add( e, f->m, rargs[ RA_NR_ARGS ], rargs[ RA_NR_ARGS ], OP_TARGETIMMED( 8 + 8 * vregbase ) );
		mop->sub( e, f->m, sp, fp, rargs[ RA_NR_ARGS ] );	
	}
	
	// release temps used in call
	release_temp( mop, e, f->m );
	prefer_nontemp_release_reg( mop, e, f->m, RA_COUNT );
}

void do_call( struct machine_ops* mop, struct emitter* e, struct frame* f, int vregbase, int narg, int nret ){
	caller_prologue( mop, e, f, vregbase, narg, nret );
	caller_epilogue( mop, e, f, vregbase, narg, nret );
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
		save_frame_limit( mop, e, f, vregbase, nret - 1 );	// NOT SIZE BUT LAST

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

void copy_args( struct machine_ops* mop, struct emitter* e, struct frame* f, operand rargs[ RA_COUNT ] ){
	const operand fp = OP_TARGETREG( f->m->fp );

	// copy args, need two temps that must be live simultenously
	bool prior = disable_spill( f->m );
	operand iter = OP_TARGETREG( acquire_temp( mop, e, f->m ) );
	operand dst = OP_TARGETREG( acquire_temp( mop, e, f->m ) );
	restore_spill( f->m, prior );	

	mop->move( e, f->m, iter, OP_TARGETIMMED( 0 ) );
	mop->add( e, f->m, dst, fp, OP_TARGETIMMED( -8 ) );				// skip over stored base pointer 
	mop->add( e, f->m, rargs[ RA_BASE ], rargs[ RA_BASE ], OP_TARGETIMMED( -8 ) );	// base should be +8 so skip over closure

	e->ops->label_local( e, 0 );
	mop->beq( e, f->m, iter, rargs[ RA_NR_ARGS ], LBL_NEXT( 0 ) );  
	
	// copy type and value
	mop->move( e, f->m, OP_TARGETDADDR( dst.reg, 0 ), OP_TARGETDADDR( rargs[ RA_BASE ].reg, 0 ) );
	mop->move( e, f->m, OP_TARGETDADDR( dst.reg, 4 ), OP_TARGETDADDR( rargs[ RA_BASE ].reg, 4 ) );
	
	// update pointers 
	mop->add( e, f->m, rargs[ RA_BASE ], rargs[ RA_BASE ], OP_TARGETIMMED( -8 ) );
	mop->add( e, f->m, dst, dst, OP_TARGETIMMED( -8 ) ); 
	
	// update iterator
	mop->add( e, f->m, iter, iter, OP_TARGETIMMED( 1 ) );

	mop->b( e, f->m, LBL_PREV( 0 ) );
	e->ops->label_local( e, 0 );

	release_temp( mop, e, f->m );
	release_temp( mop, e, f->m );
}

void prologue( struct machine_ops* mop, struct emitter* e, struct frame* f ){
	// new frame assumes no temporaries have been used yet 
	assert( temps_accessed( f->m ) == 0 );
	assert( RA_COUNT <= f->m->nr_reg );		// regargs are passed by register NOT stack

	const operand sp = OP_TARGETREG( f->m->sp );
	const operand fp = OP_TARGETREG( f->m->fp );

	// get arg passing registers
	operand rargs[ RA_COUNT ];
	prefer_nontemp_acquire_reg( mop, e, f->m, RA_COUNT, rargs );

	// push old frame pointer, closure addr / result start addr, expected nr or results 
	pushn( mop, e, f->m, 2, fp, rargs[ RA_BASE ] ); 

	// set ebp and update stack
	mop->add( e, f->m, fp, sp, OP_TARGETIMMED( 4 ) );	// point to ebp so add 4 
	mop->add( e, f->m, sp, sp, OP_TARGETIMMED( -( 8 * f->nr_locals ) ) ); 

	if( f->nr_params ){
		copy_args( mop, e, f, rargs );
		// TODO: copy nil
	 	load_frame_limit( mop, e, f, 0, f->nr_params );	// load locals living in registers 
	}

	prefer_nontemp_release_reg( mop, e, f->m, RA_COUNT );
}

void epilogue( struct machine_ops* mop, struct emitter* e, struct frame* f ){
	const operand sp = OP_TARGETREG( f->m->sp );
	const operand fp = OP_TARGETREG( f->m->fp );


	// reset stack 
	mop->move( e, f->m, sp, fp );
	pop( mop, e, f->m, fp );
	mop->ret( e, f->m );
}

#if 0		// doesn't work because return address will overwrite and some arch ( ahem! x86 ) haven no alternatives 


static void load_args( struct machine_ops* mop, struct emitter* e, struct frame* f, operand rargs[ RA_COUNT ] ){
	operand sp = OP_TARGETREG( f->m->sp );
	operand t0 = OP_TARGETREG( acquire_temp( mop, e, f->m ) );

	/*
	* n = nr_locals 
	* f(i) = sp + 8( n - ( i + 1 ) )		// argument addr in new frame 
	* g(i) = base - 8i 				// argument addr in old frame
	* f(i) - g(i) = delta = sp - base + 8( n - 1 )	
	*/

	// calculate f( NR_PARAMS - 1 )
	mop->add( e, f->m, t0, sp, OP_TARGETIMMED( 8 * ( f->nr_locals - f->nr_params ) ) );

	// base = g( NR_ARGS - 1 );  NR_ARGS = f( NR_ARGS - 1 );
	mop->mul( e, f->m, rargs[ RA_NR_ARGS ], rargs[ RA_NR_ARGS ], OP_TARGETIMMED( 8 ) );
	mop->add( e, f->m, rargs[ RA_BASE ], rargs[ RA_BASE ], rargs[ RA_NR_ARGS ] );
	mop->add( e, f->m, rargs[ RA_BASE ], rargs[ RA_BASE ], OP_TARGETIMMED( -8 ) );
	mop->add( e, f->m, rargs[ RA_NR_ARGS ], rargs[ RA_NR_ARGS ], sp );
	mop->add( e, f->m, rargs[ RA_NR_ARGS ], rargs[ RA_NR_ARGS ], OP_TARGETIMMED( 8 * f->nr_locals ) );


	// loop storing nil in type 
	e->ops->label_local( e, 0 );
	mop->bge( e, f->m, t0, rargs[ RA_NR_ARGS ], LBL_NEXT( 0 )  );
	mop->move( e, f->m, OP_TARGETDADDR( t0.reg, 4 ), OP_TARGETIMMED( LUA_TNIL ) ); 
	mop->add( e, f->m, t0, t0, OP_TARGETIMMED( 8 ) ); 
	mop->b( e, f->m, LBL_PREV( 0 ) );
	e->ops->label_local( e, 0 );

	// NR_ARGS = f( 0 ), not needed anymore
	mop->add( e, f->m, rargs[ RA_NR_ARGS ], sp, OP_TARGETIMMED( 8 * ( f->nr_params - 1 ) ) ); 

	
	// copy args in reverse order ( avoid overwrite )  
	e->ops->label_local( e, 0 );
	mop->bge( e, f->m, t0, rargs[ RA_NR_ARGS ], LBL_NEXT( 0 ) );

	mop->move( e, f->m, OP_TARGETDADDR( t0.reg, 0 ), OP_TARGETDADDR( rargs[ RA_BASE ].reg, 0 ) );
	mop->move( e, f->m, OP_TARGETDADDR( t0.reg, 4 ), OP_TARGETDADDR( rargs[ RA_BASE ].reg, 4 ) );

	mop->add( e, f->m, rargs[ RA_BASE ], rargs[ RA_BASE ], OP_TARGETIMMED( 8 ) );	
	mop->add( e, f->m, t0, t0, OP_TARGETIMMED( 8 ) ); 
	mop->b( e, f->m, LBL_PREV( 0 ) );
	e->ops->label_local( e, 0 );

	// release the temp iterator 
	release_temp( mop, e, f->m );

}

void prologue( struct machine_ops* mop, struct emitter* e, struct frame* f ){
	const int meta_off = f->nr_locals * 8;

	// new frame assumes no temporaries have been used yet 
	assert( temps_accessed( f->m ) == 0 );
	assert( RA_COUNT <= f->m->nr_reg );		// regargs are passed by register NOT stack

	// first things first sub stack
	operand sp = OP_TARGETREG( f->m->sp );
	bool prior = disable_spill( f->m );	
	mop->add( e, f->m, sp, sp, OP_TARGETIMMED( -stack_frame_size( f->nr_locals ) ) ); 
	restore_spill( f->m, prior );

	// get arg passing registers
	operand rargs[ RA_COUNT ];
	prefer_nontemp_acquire_reg( mop, e, f->m, RA_COUNT, rargs );

	// do this first its very delicate 
	if( f->nr_params > 0 )			// TODO: obivously change to > 0 once intial testing done
		load_args( mop, e, f, rargs );

	// store metaframe section
	mop->move( e, f->m, OP_TARGETDADDR( f->m->sp, meta_off ), rargs[ RA_NR_RESULTS ] );
	mop->move( e, f->m, OP_TARGETDADDR( f->m->sp, meta_off + 4 ), rargs[ RA_BASE ] );
	mop->move( e, f->m, OP_TARGETDADDR( f->m->sp, meta_off + 8 ), rargs[ RA_CLOSURE ] );

	prefer_nontemp_release_reg( mop, e, f->m, RA_COUNT );


	// load live registers  
	load_frame( mop, e, f );
}

void epilogue( struct machine_ops* mop, struct emitter* e, struct frame* f ){
	const int meta_off = f->nr_locals * 8;
	
	// new frame assumes no temporaries have been used yet 
	assert( temps_accessed( f->m ) == 0 );
	assert( RA_COUNT <= f->m->nr_reg );		// regargs are passed by register NOT stack

	// get arg passing registers
	operand rargs[ RA_COUNT ];
	prefer_nontemp_acquire_reg( mop, e, f->m, RA_COUNT, rargs );

	// load metaframe section
	mop->move( e, f->m, rargs[ RA_NR_RESULTS ], OP_TARGETDADDR( f->m->sp, meta_off ) );
	mop->move( e, f->m, rargs[ RA_BASE ], OP_TARGETDADDR( f->m->sp, meta_off + 4 ) );
	mop->move( e, f->m, rargs[ RA_CLOSURE ], OP_TARGETDADDR( f->m->sp, meta_off + 8 ) );

	prefer_nontemp_release_reg( mop, e, f->m, RA_COUNT );

	// move stack to prior frame
	operand sp = OP_TARGETREG( f->m->sp );
	mop->add( e, f->m, sp, sp, OP_TARGETIMMED( stack_frame_size( f->nr_locals ) ) );

	mop->ret( e, f->m );
}


void do_call( struct machine_ops* mop, struct emitter* e, struct frame* f, int vregbase, int narg, int nret ){
	// new frame assumes no temporaries have been used yet 
	assert( temps_accessed( f->m ) == 0 );
	assert( RA_COUNT <= f->m->nr_reg );		// regargs are passed by register NOT stack

	vreg_operand clive = vreg_to_operand( f, vregbase, false );
 	vreg_operand cstack = vreg_to_operand( f, vregbase, true );
	assert( cstack.value.tag == OT_DIRECTADDR );

	// TODO: verify its a closure 
	
	// get arg passing registers
	operand rargs[ RA_COUNT ];
	prefer_nontemp_acquire_reg( mop, e, f->m, RA_COUNT, rargs );

	if( narg > 0 )
		mop->move( e, f->m, rargs[ RA_NR_ARGS ], OP_TARGETIMMED( narg - 1 ) );

	mop->move( e, f->m, rargs[ RA_NR_RESULTS ], OP_TARGETIMMED( nret ) );
	mop->move( e, f->m, rargs[ RA_CLOSURE ], clive.value );

	// calculate base address 
	mop->add( e, f->m, rargs[ RA_BASE ], OP_TARGETREG( f->m->sp ), OP_TARGETIMMED( cstack.value.offset ) );
	
	mop->	

	prefer_nontemp_release_reg( mop, e, f->m, RA_COUNT );
}

#endif 
