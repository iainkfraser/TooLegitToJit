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
enum REGARGS { RA_NR_ARGS, RA_BASE, RA_NR_RESULTS, RA_CLOSURE, RA_COUNT };

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
	// TODO: disable temps
	mop->add( e, f->m, sp, sp, OP_TARGETIMMED( -stack_frame_size( f->nr_locals ) ) ); 
	// TODO: enable temps 

	// get arg passing registers
	operand rargs[ RA_COUNT ];
	prefer_nontemp_acquire_reg( mop, e, f->m, RA_COUNT, rargs );

	// do this first its very delicate 
	if( f->nr_params >= 0 )			// TODO: obivously change to > 0 once intial testing done
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

