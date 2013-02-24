/*
* (C) Iain Fraser - GPLv3 
*
* MIPS mapping of 
*	virtual stack <=> registers or stack  
*/

#include <stdlib.h>
#include <stdio.h>
#include "math_util.h"
#include "mc_emitter.h"
#include "arch/mips/regdef.h"
#include "arch/mips/opcodes.h"
#include "arch/mips/emitter.h"
#include "arch/mips/regmap.h"

/*
* OLD CODE BELOW 
*/

/*
* Compile-time virtual register mapping 
*/



int nr_livereg_vreg_occupy( int nr_locals ){
	return 2 * nr_locals;
}


/*
* Runtime virtual register mapping. Stack functions will store the address in the 
* out register.  
*/

void vreg_runtime_reg_value( struct arch_emitter* me, int nr_locals, int ridx, int rout ){
	assert( false );	
}

void vreg_runtime_reg_type( int nr_locals, int vreg ){
	assert( false );
}


void vreg_runtime_stack_value( struct arch_emitter* me, int nr_locals, int ridx, int rout ){
	// i = i + 1	# start 1 position after calling position
	// i << 3	# 8i
	// -i		# -8i  
	// i += 8(n-1)	# 8(n-1) - 8i
	// rout = sp + i 
	EMIT( MI_ADDIU( rout, ridx, 1 ) );
	EMIT( MI_SLL( rout, rout, 3 ) );
	EMIT( MI_NEGU( rout, rout ) );
	EMIT( MI_ADDIU( rout, rout, 8 * ( nr_locals - 1 ) ) );
	EMIT( MI_ADDU( rout, _sp, rout ) ); 
}
/*
void vreg_runtime_stack_type( struct arch_emitter* me, int nr_locals, int ridx, int rout ){
	EMIT( MI_ADDIU( rout, ridx, 1 ) );
	EMIT( MI_SLL( rout, rout, 3 ) );
	EMIT( MI_SUBU( rout, _zero, rout ) );
	EMIT( MI_ADDIU( rout, rout, 8 * ( nr_locals - 1 ) + 4 ) );
	EMIT( MI_ADDU( rout, _sp, rout ) ); 
}*/

void vreg_runtime_stack_value_load( struct arch_emitter* me, int rstack, int roff, int ioff, int rout ){
	if( roff != _zero ){	 
		EMIT( MI_ADDU( rout, rstack, roff ) );
		rstack = rout;
	}

	EMIT( MI_LW( rout, rstack, ioff * -8 ) ); 
}

// take stack offset 
void vreg_runtime_stack_type_load( struct arch_emitter* me, int rstack, int roff, int ioff, int rout ){
	if( roff != _zero ){	 
		EMIT( MI_ADDU( rout, rstack, roff ) );
		rstack = rout;
	}

	EMIT( MI_LW( rout, rstack, ioff * -8 + 4 ) ); 
}


void vreg_runtime_stack_value_store( struct arch_emitter* me, int rstack, int roff, int ioff, int rvalue ){
	int base = rstack;

	if( roff != _zero ){	 
		EMIT( MI_ADDU( roff, rstack, roff ) );
		base = roff;
	}

	EMIT( MI_SW( rvalue, base, ioff * -8 ) );

	if( roff != _zero )
		 EMIT( MI_SUBU( roff, roff, rstack ) );

}

void vreg_runtime_stack_type_store( struct arch_emitter* me, int rstack, int roff, int ioff, int rvalue ){
	int base = rstack;

	if( roff != _zero ){	 
		EMIT( MI_ADDU( roff, rstack, roff ) );
		base = roff;
	}

	EMIT( MI_SW( rvalue, base, ioff * -8 + 4 ) );

	if( roff != _zero )
		 EMIT( MI_SUBU( roff, roff, rstack ) );
}


