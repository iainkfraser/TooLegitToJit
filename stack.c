/*
* (C) Iain Fraser - GPLv3 
*
* Platform independent Lua stack manipulation and mapping functions.
*/

#include <stdbool.h>
#include <stdint.h>
#include "math_util.h"
#include "operand.h"
#include "stack.h"
#include "frame.h"

/*
* Save and reload stack frames
*/

void x_frame( struct machine_ops* mop, struct emitter* me, struct frame* f, bool isstore, int off, int n ){
//	operand st,sv,dt,dv;
	vreg_operand s,d;
	
	const int nr_locals = f->nr_locals; 
	n += off;

	for( int i = off; i < n; i++ ){
		d = vreg_to_operand( f, i, true );
		s = vreg_to_operand( f, i, false );

		if( !isstore )
			swap( d, s );
				
		for( int i = 0; i < 2; i++ ){
			if( s.o[i].tag == OT_REG || d.o[i].tag == OT_REG )
				mop->move( me, f->m, d.o[i], s.o[i] );
		}


	}
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

