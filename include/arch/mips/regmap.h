/*
* (C) Iain Fraser - GPLv3  
* MIPS register mapping for Lua.  
*/

#ifndef _MIPS_MAPPING_H_
#define _MIPS_MAPPING_H_

#include "math_util.h"
#include <assert.h>

#define NR_REGISTERS		18

/* temporaries available */
#define TEMP_REG0	_a0
#define TEMP_REG1	_a1
#define TEMP_REG2	_a2
#define DATA_REG	_AT	// store pointer to data section


static int vreg_to_physical_reg( int vreg ){

	if( vreg < 8 )
		return  _t0 + vreg;
	else if ( vreg < 10 )
		return _t8 + ( vreg - 8 );
	else if ( vreg < 18 )
		return _s0 + ( vreg - 10 );
	else 
		assert( false );
}



#endif
