/*
* (C) Iain Fraser - GPLv3  
* Utility functions for mapping between different data spaces.
*/

#ifndef _MIPS_MAPPING_H_
#define _MIPS_MAPPING_H_

#include "math_util.h"
#include <assert.h>

#define NR_REGISTERS		18
#define NR_TAGS_IN_WORD		( ( sizeof( int ) * 8 ) / NR_TAG_BITS ) 	

#define TEMP_REG0	_a0
#define TEMP_REG1	_a1
#define TEMP_REG2	_a2
#define DATA_REG	_at	// store pointer to data section

static inline int tag_count( int nr_locals ){
	return int_ceil_div( nr_locals, NR_TAGS_IN_WORD );
}

static inline int local_vreg_count( int nr_locals ){
	return tag_count( nr_locals ) + nr_locals;
}

static inline int local_vreg( int local ){
	return tag_count( local + 1 ) + local;
}

static inline int total_vreg_count( int nr_locals, int nr_const ){
	return local_vreg_count( nr_locals ) + nr_const;
}

static inline int local_spill( int nr_locals ){
	const int vregs = local_vreg_count( nr_locals ); 
	return max( 0, vregs - NR_REGISTERS );
} 

static inline int const_spill( int nr_locals, int nr_const ){
	const int vregs = total_vreg_count( nr_locals, nr_const );
	return max( 0, vregs - NR_REGISTERS );
}


static inline int vreg_to_reg( int vreg ){
	assert( vreg < NR_REGISTERS );
	
	if( vreg < 8 )
		return _t0 + vreg;
	else if ( vreg < 10 )
		return _t8 + ( vreg - 8 );
	else if ( vreg < 18 )
		return _s0 + ( vreg - 10 );
	else 
		assert( false );
}

#endif
