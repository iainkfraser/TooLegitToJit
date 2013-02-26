/*
* (C) Iain Fraser - GPLv3 
*
* Lua function prologue and epilogue code generation. 

* Functions are called after Lua constants are loaded ( see frame.c )
* and processed.  
*/

#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include "frame.h"
#include "machine.h"
#include "lopcodes.h"
#include "synthetic.h"

static void emit_load_params( struct machine_ops* mop, struct emitter* me, struct frame* f, const int reg_value, const int reg_count );

static int stack_frame_size( int nr_locals ){
	int k = 4 * 4;	// ra, closure, return position, nr results 
	return nr_locals * 8 + k;
}

void epilogue( struct machine_ops* mop, struct emitter* e, struct frame* f ){
#if 0
	const int rsp = f->m->sp;
	operand osp = OP_TARGETREG( rsp );

	int temp1 = acquire_specfic_temp( f->m, 0 );
	int temp2 = acquire_specfic_temp( f->m, 1 );

	mop->add( e, f->m, osp, osp, OP_TARGETIMMED( stack_frame_size( f->nr_locals ) ) ); 
	mop->move( e, f->m, OP_TARGETREG( temp1 ), OP_TARGETDADDR( rsp, -4 ) ); 
	mop->move( e, f->m, OP_TARGETREG( temp2 ), OP_TARGETDADDR( rsp, -8 ) ); 

	release_specfic_temp( f->m, 0 );
	release_specfic_temp( f->m, 1 );

	mop->ret( e, f->m );
#endif
}


void prologue( struct machine_ops* mop, struct emitter* e, struct frame* f ){
#if 0
	const int rsp = f->m->sp;
	operand osp = OP_TARGETREG( rsp );

	// save closure, result address and expected number of results 
	int temp1 = acquire_specfic_temp( f->m, 0 ); 
	int temp2 = acquire_specfic_temp( f->m, 1 ); 
	int temp3 = acquire_specfic_temp( f->m, 2 ); 
	
	// load the paramaters, while still in old stack frame  
	emit_load_params( mop, e, f, temp2, temp3 );

	mop->move( e, f->m, OP_TARGETDADDR( rsp, -4 ), OP_TARGETREG( temp1 ) ); 
	mop->move( e, f->m, OP_TARGETDADDR( rsp, -8 ), OP_TARGETREG( temp2 ) ); 
	mop->move( e, f->m, OP_TARGETDADDR( rsp, -12 ), OP_TARGETREG( temp3 ) ); 

	release_specfic_temp( f->m, 0 );
	release_specfic_temp( f->m, 1 );
	release_specfic_temp( f->m, 2 );

	mop->add( e, f->m, osp, osp, OP_TARGETIMMED( -stack_frame_size( f->nr_locals ) ) ); 
#endif
}

#if 0
// can overwrite rn because its not used afterwards 
static void emit_nil_args( struct machine_ops* mop, struct emitter* e, struct frame* f, int basereg, int rn ){

#if 0
	basereg = ( basereg + 4 ) + ( 8 * rn );
	end = ( basereg + 4 ) + ( 8 * nr_param );
	while( basereg < end ){
		sw nil, basereg
		basereg += 8;
	}
	basereg = end - 4 - ( 8 * nr_param );
#endif

#if 0
	operand iter = OP_TARGETREG( rn );
	operand base = OP_TARGETREG( basereg );

	e->ops->label_local( e, 0 );
	mop->beq( e, f->m, iter, OP_TARGETIMMED( f->nr_params ), LBL_NEXT( 1 ) ); 
	mop->add( e, f->m, base, iter
	mop->add( e, f->m, iter, iter, OP_TARGETIMMED( 1 ) ); 
	mop->b( e, f->m, LBL_NEXT( 0 ) );	
	e->ops->label_local( e, 1 );
#endif 
}

static void emit_copy_param( struct machine_ops* mop, struct emitter* me, struct frame* f, vreg_operand dst, int basereg, int idx ){
	vreg_operand src;
	src.value = OP_TARGETDADDR( basereg, idx * 8 );
	src.type = OP_TARGETDADDR( basereg, idx * 8 + 4 );

	assign( mop, me, f->m, dst, src );
}

/*
* temp[0] = stack 
* temp[1] = address to store results and address - 1 of argument location 
* temp[2] = number of arguments available
* temp[3] = number of ret args
* stack is still in the old stack frame  
*/
static void emit_load_params( struct machine_ops* mop, struct emitter* me, struct frame* f, const int reg_value, const int reg_count ){
	operand odst_value, odst_type, osrc_value, osrc_type;
	vreg_operand dst;

	// set unpassed arguments to nil  	
	emit_nil_args( mop, me, f, reg_value, reg_count );

	// TODO: add acquire_temp to emitter which takes machine as arg? 	
	for( int i = 0; i < f->nr_params; i++ ){
		dst = vreg_to_operand( f, i, false );
		emit_copy_param( mop, me, f, dst, reg_value, i );
	}	
}
#endif 
