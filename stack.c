/*
* (C) Iain Fraser - GPLv3 
*
* Platform independent Lua stack manipulation and mapping functions.
*/

#include "math_util.h"
#include "stack.h"

/*
* Save and reload stack frames
*/

void x_frame( struct arch_emitter* me, bool isstore ){
//	operand st,sv,dt,dv;
	vreg_operand s,d;
	
	const int nr_locals = arch_nr_locals( me );

	for( int i =0; i < nr_locals; i++ ){
		d = arch_vreg_to_operand( nr_locals, i, true );
		s = arch_vreg_to_operand( nr_locals, i, false );

		if( !isstore )
			swap( d, s );
				
		for( int i = 0; i < 2; i++ ){
			if( s.o[i].tag == OT_REG )
				arch_move( me, d.o[i], s.o[i] );
		}


	}
}

void store_frame( struct arch_emitter* me ){
	x_frame( me, true );
}

void load_frame( struct arch_emitter* me ){
	x_frame( me, false );
	// TODO: reload the constants 
}


