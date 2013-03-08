/*
* (C) Iain Fraser - GPLv3 
*
* Platform independent Lua stack manipulation and mapping functions.
*/

#include <stdbool.h>
#include <stdint.h>
#include "macros.h"
#include "operand.h"
#include "stack.h"
#include "frame.h"
#include "jitfunc.h"

/*
* Save and reload stack frames
*/

int x_frame( struct machine_ops* mop, struct emitter* me, struct frame* f, bool isstore, int off, int n ){
//	operand st,sv,dt,dv;
	vreg_operand s,d;
	int end = off + n;
	int spill = 0;
#if 0
	for( int i = off; i < n; i++ ){
#else
	for( int i = end - 1; i >= off; i-- ){
#endif
		d = vreg_to_operand( f, i, true );
		s = vreg_to_operand( f, i, false );

		if( !isstore )
			swap( d, s );
				
		for( int i = 0; i < 2; i++ ){
			if( s.o[i].tag == OT_REG || d.o[i].tag == OT_REG ){
				mop->move( me, f->m, d.o[i], s.o[i] );
				spill++;
			}
		}


	}

	return spill;
}

void store_frame( struct machine_ops* mop, struct emitter* e, struct frame* f ){
	x_frame( mop, e, f, true, 0, f->nr_locals );
}

void load_frame( struct machine_ops* mop, struct emitter* e, struct frame* f ){
	x_frame( mop, e, f, false, 0, f->nr_locals );
	// TODO: reload the constants 
}

void load_frame_limit( struct machine_ops* mop, struct emitter* e, struct frame* f, int off, int n ){
	x_frame( mop, e, f, false, off, n );
}

void save_frame_limit( struct machine_ops* mop, struct emitter* e, struct frame* f, int off, int n ){
	x_frame( mop, e, f, true, off, n );
}


/*
* Jit functions for storing/loading registers. Used before Lua call.
*/

static int max_live_locals( struct machine* m ){
	return int_ceil_div( m->nr_reg - m->nr_temp_regs, 2 );
}

static void jinit_x_access( struct machine_ops* mop, struct JFunc* jf, struct emitter* e, struct machine* m, bool isstore ){
	int start,end, spill;

	// phoney frame 
	struct frame f = { .m = m, .nr_locals = max_live_locals( m ), .nr_params = 0 };
	
	// access stack and return
	start =  e->ops->ec( e );
	spill = x_frame( mop, e, &f, isstore, 0, f.nr_locals );
	end = e->ops->ec( e ); 

	// set size of single op
	assert( ( end - start ) % spill == 0 );
	jf->data = ( end - start ) / spill;	

	mop->ret( e, m );
}

void jinit_store_regs( struct JFunc* jf, struct machine_ops* mop, struct emitter* e, struct machine* m ){
	assert( m->nr_reg > m->nr_temp_regs );
	jinit_x_access( mop, jf, e, m, true );
}


void jinit_load_regs( struct JFunc* jf, struct machine_ops* mop, struct emitter* e, struct machine* m ){
	jinit_x_access( mop, jf, e, m, false );
}

int jf_loadlocal_offset( struct machine* m, int nr_locals ){
	struct JFunc* j = jfuncs_get( JF_LOAD_LOCALS );
	return max( 0, ( m->nr_reg - m->nr_temp_regs ) - 2 * nr_locals ) * j->data; 
}

int jf_storelocal_offset( struct machine* m, int nr_locals ){
	struct JFunc* j = jfuncs_get( JF_STORE_LOCALS );
	return max( 0, ( m->nr_reg - m->nr_temp_regs ) - 2 * nr_locals ) * j->data; 
}
