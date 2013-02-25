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

void x_frame( struct machine_ops* mop, struct emitter* me, struct frame* f, bool isstore ){
//	operand st,sv,dt,dv;
	vreg_operand s,d;
	
	const int nr_locals = f->nr_locals; 

	for( int i =0; i < nr_locals; i++ ){
		d = vreg_to_operand( f, i, true );
		s = vreg_to_operand( f, i, false );

		if( !isstore )
			swap( d, s );
				
		for( int i = 0; i < 2; i++ ){
			if( s.o[i].tag == OT_REG )
				mop->move( me, f->m, d.o[i], s.o[i] );
		}


	}
}

void store_frame( struct machine_ops* mop, struct emitter* e, struct frame* f ){
	x_frame( mop, e, f, true );
}

void load_frame( struct machine_ops* mop, struct emitter* e, struct frame* f ){
	x_frame( mop, e, f, false );
	// TODO: reload the constants 
}


