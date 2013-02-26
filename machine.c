/*
* (C) Iain Fraser - GPLv3  
* Physical machine interface. Utility functions. 
*/

#include <stdbool.h>
#include "machine.h"

#define TEMP_FLAG	0x10000


static int do_acq_temp( struct machine* m, int i ){
	if( m->reg[ i ] & TEMP_FLAG )	return -1;
	int reg = m->reg[ i ];
	m->reg[ i ] |= TEMP_FLAG;
	return reg;
}

int acquire_specfic_temp( struct machine* m, int idx ){
	int reg;

	if( ( reg = do_acq_temp( m, idx ) ) != -1 )
		return reg;

	assert( false );	// TODO: proper error handling	
}

int acquire_temp( struct machine* m ){
	int reg;

	for( int i = 0; i < m->nr_temp_regs; i++ ){
		if( ( reg = do_acq_temp( m, i ) ) != -1 )
			return reg;
	}

	assert( false );		// TODO: proper error handling 
	return 0;
}

void release_temp( struct machine* m, int reg ){
	for( int i = 0; i < m->nr_temp_regs; i++ ){
		if(  ( m->reg[ i ] & ~TEMP_FLAG ) == reg  ){	
			m->reg[ i ] &= ~TEMP_FLAG;
			return;
		}
	}

	assert( false );
}


void release_specfic_temp( struct machine *m, int idx ){

}
