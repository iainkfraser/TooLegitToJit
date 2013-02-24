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

void x_frame( struct arch_emitter* me, struct frame* f, bool isstore ){
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
				arch_move( me, d.o[i], s.o[i] );
		}


	}
}

void store_frame( struct arch_emitter* me, struct frame* f ){
	x_frame( me, f, true );
}

void load_frame( struct arch_emitter* me, struct frame* f ){
	x_frame( me, f, false );
	// TODO: reload the constants 
}


