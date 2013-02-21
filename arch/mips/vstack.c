/*
* (C) Iain Fraser - GPLv3 
*
* MIPS mapping of 
*	virtual stack <=> registers or stack  
*/

#include <stdlib.h>
#include <stdio.h>
#include "mc_emitter.h"
#include "arch/mips/regdef.h"
#include "arch/mips/opcodes.h"
#include "arch/mips/emitter.h"
#include "arch/mips/mapping.h"


/*
* Compile-time virtual register mapping 
*/

#define INVERSE( i, n )	( n - ( i + 1 ) )

operand vreg_compiletime_stack_value( int nr_locals, int vreg ){
	operand r = OP_TARGETDADDR( _sp, 8 * INVERSE( vreg, nr_locals ) );  	
	return r;
}

operand vreg_compiletime_stack_type( int nr_locals, int vreg ){
	operand r = OP_TARGETDADDR( _sp, 8 * INVERSE( vreg, nr_locals ) + 4 );  	
	return r;
}


static operand idx_physical_reg( int vreg ){
	int r;

	if( vreg < 8 )
		r =  _t0 + vreg;
	else if ( vreg < 10 )
		r = _t8 + ( vreg - 8 );
	else if ( vreg < 18 )
		r = _s0 + ( vreg - 10 );
	else 
		assert( false );


	operand ret =  OP_TARGETREG( r );
	return ret;
}

operand vreg_compiletime_reg_value( int nr_locals, int vreg ){
	const int pidx = vreg * 2;
	return pidx < NR_REGISTERS ?
			idx_physical_reg( pidx ) :
			vreg_compiletime_stack_value( nr_locals, vreg );
}

operand vreg_compiletime_reg_type( int nr_locals, int vreg ){
	const int pidx = vreg * 2 + 1;	
	return pidx < NR_REGISTERS ?
			idx_physical_reg( pidx ) :
			vreg_compiletime_stack_value( nr_locals, vreg );
}

/*
* Runtime virtual register mapping. Stack functions will store the address in the 
* out register.  
*/

void vreg_runtime_reg_value( struct mips_emitter* me, int nr_locals, int ridx, int rout ){
	assert( false );	
}

void vreg_runtime_reg_type( int nr_locals, int vreg ){
	assert( false );
}


void vreg_runtime_stack_value( struct mips_emitter* me, int nr_locals, int ridx, int rout ){
	// i << 3	# 8i
	// ~i		# -8i  
	// i += 8(n-1)	# 8(n-1) - 8i
	// rout = sp + i 
	EMIT( MI_SLL( rout, ridx, 3 ) );
	EMIT( MI_NOT( rout, rout ) );
	EMIT( MI_ADDIU( rout, rout, 8 * ( nr_locals - 1 ) ) );
	EMIT( MI_ADDU( rout, _sp, rout ) ); 
}

void vreg_runtime_stack_type( struct mips_emitter* me, int nr_locals, int ridx, int rout ){
	EMIT( MI_SLL( rout, ridx, 3 ) );
	EMIT( MI_NOT( rout, rout ) );
	EMIT( MI_ADDIU( rout, rout, 8 * ( nr_locals - 1 ) + 4 ) );
	EMIT( MI_ADDU( rout, _sp, rout ) ); 
}

void vreg_runtime_stack_value_load( struct mips_emitter* me, int rstack, int roff, int ioff, int rout ){
	if( roff != _zero ){	 
		EMIT( MI_ADDU( rout, rstack, roff ) );
		rstack = rout;
	}

	EMIT( MI_LW( rout, rstack, ioff * 8 ) ); 
}

void vreg_runtime_stack_type_load( struct mips_emitter* me, int rstack, int roff, int ioff, int rout ){
	vreg_runtime_stack_value_load( me, rstack, roff, ioff, rout );
}


void vreg_runtime_stack_value_store( struct mips_emitter* me, int rstack, int roff, int ioff, int rvalue ){
	int base = rstack;

	if( roff != _zero ){	 
		EMIT( MI_ADDU( roff, rstack, roff ) );
		base = roff;
	}

	EMIT( MI_SW( rvalue, base, ioff * 8 ) );

	if( roff != _zero )
		 EMIT( MI_SUBU( roff, roff, rstack ) );

}

void vreg_runtime_stack_type_store( struct mips_emitter* me, int rstack, int roff, int ioff, int rvalue ){
	vreg_runtime_stack_value_store( me, rstack, roff, ioff, rvalue );	
}

/*
* Old Shitty Code 
*/

#if 1  

int tag_count( int nr_locals ){
	return int_ceil_div( nr_locals, NR_TAGS_IN_WORD );
}

int local_vreg_count( int nr_locals ){
	return tag_count( nr_locals ) + nr_locals;
}

int local_vreg( int local ){
	return tag_count( local + 1 ) + local;
}

int total_vreg_count( int nr_locals, int nr_const ){
	return local_vreg_count( nr_locals ) + nr_const;
}

int local_spill( int nr_locals ){
	const int vregs = local_vreg_count( nr_locals ); 
	return max( 0, vregs - NR_REGISTERS );
} 

int const_spill( int nr_locals, int nr_const ){
	const int vregs = total_vreg_count( nr_locals, nr_const );
	return max( 0, vregs - NR_REGISTERS );
}


int vreg_to_reg( int vreg ){
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


operand local_to_operand( struct mips_emitter* me, int l ){
	operand r;

	int vreg = local_vreg( l );
	printf("l:%d v:%d\n", l, vreg );

	if( vreg < NR_REGISTERS ) {	
		r.tag = OT_REG;
		r.reg = vreg_to_reg( vreg );
	} else { 
		r.tag = OT_DIRECTADDR;
		r.base = _sp;
		r.offset = 8 + ( vreg - NR_REGISTERS );		// saved registers stored at bottom of stack hence +8 
	}

	return r;
}


operand luaoperand_to_operand( struct mips_emitter* me, loperand op ){
	return op.islocal ? local_to_operand( me, op.index )  : const_to_operand( me, op.index );
}


int nr_slots( struct mips_emitter* me ){
	const int locals = me->tregs - me->cregs;	// locals need tag
	return local_vreg_count( locals ) + me->cregs;	
}

#endif 
