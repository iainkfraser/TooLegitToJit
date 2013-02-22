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

int nr_livereg_vreg_occupy( int nr_locals ){
	return 2 * nr_locals;
}

static operand idx_physical_reg( int vreg ){
	operand ret =  OP_TARGETREG( vreg_to_physical_reg( vreg ) );
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
void vreg_runtime_stack_type( struct mips_emitter* me, int nr_locals, int ridx, int rout ){
	EMIT( MI_ADDIU( rout, ridx, 1 ) );
	EMIT( MI_SLL( rout, rout, 3 ) );
	EMIT( MI_SUBU( rout, _zero, rout ) );
	EMIT( MI_ADDIU( rout, rout, 8 * ( nr_locals - 1 ) + 4 ) );
	EMIT( MI_ADDU( rout, _sp, rout ) ); 
}*/

void vreg_runtime_stack_value_load( struct mips_emitter* me, int rstack, int roff, int ioff, int rout ){
	if( roff != _zero ){	 
		EMIT( MI_ADDU( rout, rstack, roff ) );
		rstack = rout;
	}

	EMIT( MI_LW( rout, rstack, ioff * -8 ) ); 
}

// take stack offset 
void vreg_runtime_stack_type_load( struct mips_emitter* me, int rstack, int roff, int ioff, int rout ){
	if( roff != _zero ){	 
		EMIT( MI_ADDU( rout, rstack, roff ) );
		rstack = rout;
	}

	EMIT( MI_LW( rout, rstack, ioff * -8 + 4 ) ); 
}


void vreg_runtime_stack_value_store( struct mips_emitter* me, int rstack, int roff, int ioff, int rvalue ){
	int base = rstack;

	if( roff != _zero ){	 
		EMIT( MI_ADDU( roff, rstack, roff ) );
		base = roff;
	}

	EMIT( MI_SW( rvalue, base, ioff * -8 ) );

	if( roff != _zero )
		 EMIT( MI_SUBU( roff, roff, rstack ) );

}

void vreg_runtime_stack_type_store( struct mips_emitter* me, int rstack, int roff, int ioff, int rvalue ){
	int base = rstack;

	if( roff != _zero ){	 
		EMIT( MI_ADDU( roff, rstack, roff ) );
		base = roff;
	}

	EMIT( MI_SW( rvalue, base, ioff * -8 + 4 ) );

	if( roff != _zero )
		 EMIT( MI_SUBU( roff, roff, rstack ) );
}


/*
* Save and reload stack frames
*/

void x_frame( struct mips_emitter* me, bool isstore ){
	operand st,sv,dt,dv;

	for( int i =0; i < me->nr_locals; i++ ){

		sv = vreg_compiletime_reg_value( me->nr_locals, i );
		st = vreg_compiletime_reg_type( me->nr_locals, i );
		dv = vreg_compiletime_stack_value( me->nr_locals, i );
		dt = vreg_compiletime_stack_type( me->nr_locals, i );

		if( !isstore ){
			swap( sv, dv );
			swap( st, dt );
		}		

		if( sv.tag == OT_REG )
			do_assign( me, dv, sv );

		if( st.tag == OT_REG )
			do_assign( me, dt, st );

	}
}

void store_frame( struct mips_emitter* me ){
	x_frame( me, true );
}

void load_frame( struct mips_emitter* me ){
	x_frame( me, false );
	// TODO: reload the constants 
}


/*
* Map Lua constants / locals to live position.
*/

extern operand const_to_operand( struct mips_emitter* me, int k );

operand lualocal_value_to_operand( struct mips_emitter* me, int vreg ){
	return vreg_compiletime_reg_value( me->nr_locals, vreg );
}

operand lualocal_type_to_operand( struct mips_emitter* me, int vreg ){
	return vreg_compiletime_reg_type( me->nr_locals, vreg ); 
}

operand luak_value_to_operand( struct mips_emitter* me, int k ){
	return const_to_operand( me, k );
}


operand luak_type_to_operand( struct mips_emitter* me, int k ){
	assert( false );
}

operand luaoperand_value_to_operand( struct mips_emitter* me, loperand op ){
	return op.islocal ? 
		lualocal_value_to_operand( me, op.index ) :
		luak_value_to_operand( me, op.index );
}


operand luaoperand_type_to_operand( struct mips_emitter* me, loperand op ){
	return op.islocal ? 
		lualocal_type_to_operand( me, op.index ) :
		luak_type_to_operand( me, op.index );
}
