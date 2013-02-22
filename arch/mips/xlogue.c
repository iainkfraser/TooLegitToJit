/*
* (C) Iain Fraser - GPLv3 
*
* Lua function prologue and epilogue code generation. 

* Functions are called after Lua constants are loaded ( see vconsts.c )
* and processed.  
*/

#include <stdlib.h>
#include "arch/mips/regdef.h"
#include "arch/mips/opcodes.h"
#include "arch/mips/emitter.h"
#include "arch/mips/regmap.h"
#include "arch/mips/vstack.h"
#include "arch/mips/vconsts.h"
#include "arch/mips/xlogue.h"



static int stack_frame_size( int nr_locals ){
	int k = 4 * 4;	// ra, closure, return position, nr results 
	return nr_locals * 8 + k;
}

static void emit_copy_param( mips_emitter* me, int nr_locals, 
			int paramidx, operand dst, int rbase, bool isvalue ){
	assert( dst.tag == OT_REG || ( dst.tag == OT_DIRECTADDR && dst.base == _sp ) );
	
	// load result into register
	int dreg = dst.tag == OT_REG ? dst.reg : _AT; 

	if( isvalue )
		vreg_runtime_stack_value_load( me, rbase, _zero, paramidx, dreg );
	else
		vreg_runtime_stack_type_load( me, rbase, _zero, paramidx, dreg );

	// at this point still in old functions stack frame so we need to compensate. 
	if( dst.tag == OT_DIRECTADDR ){
		operand src = OP_TARGETREG( dreg );
		dst.offset -= stack_frame_size( nr_locals );
		do_assign( me, dst, src );
	}
}

static void emit_nil_args( mips_emitter* me, int nr_params, int rstackval, int rstacktype, int ridx, int rn ){
	int loopstart,loopend;

	EMIT( MI_ADDIU( _AT, _zero, TNIL ) );		// load NIL into reg for later assign
	EMIT( MI_ADDIU( rn, rn, -nr_params ) );
	
	loopstart = GET_EC( me ); 
	EMIT( MI_BGEZ( rn, 0 ) );
	EMIT( MI_NOP() );


	vreg_runtime_stack_type_store( me, rstacktype, rn, nr_params, _AT );

	EMIT( MI_B( BRANCH_TO( me, loopstart ) ) );
	EMIT( MI_ADDIU( rn, rn, 1 ) );


	// overwrite loop - now we know end of loop location 
	REEMIT( MI_BGEZ( rn, BRANCH_FROM( me, loopstart ) ), loopstart );


}
/*
* a1 + 1 = virtual register of first argument, rest follow sequentially
* a2 = number of arguments available
* stack is still in the old stack frame  
*/
static void emit_load_params( mips_emitter* me, int nr_locals, int nr_params ){
	operand odst_value, odst_type, osrc_value, osrc_type;

	// registers used in operation - don't overwrite arguments and unspilled vregs.
	const int reg_value = _v0;
	const int reg_type = _v1;


	// load argument type and value
	vreg_runtime_stack_value( me, nr_locals, _a1, reg_value );
	vreg_runtime_stack_type( me, nr_locals, _a1, reg_type );
	
	// set unpassed arguments to nil  	
	emit_nil_args( me, nr_params, reg_value, reg_type, _a1, _a2 );
	
	for( int i = 0; i < nr_params; i++ ){
		odst_value = vreg_compiletime_reg_value( nr_locals, i );		
		odst_type = vreg_compiletime_reg_type( nr_locals, i );

		emit_copy_param( me, nr_locals, i, odst_value, reg_value, true );
		emit_copy_param( me, nr_locals, i, odst_type, reg_type, false );
	}	
}

void emit_prologue( mips_emitter* me, int nr_locals, int nr_params ){
	// remember epilogue start
	me->pro = GET_EC( me );

	// load the paramaters 
	emit_load_params( me, nr_locals, nr_params );

	// save the return address, closure, return position and expected number of args
	EMIT( MI_SW( _ra, _sp, -4 ) );
	EMIT( MI_SW( _a0, _sp, -8 ) );		// closure
	EMIT( MI_SW( _a1, _sp, -12 ) );		// base result virtual register
	EMIT( MI_SW( _a3, _sp, -16 ) );		// number of results 

	// update stack
	EMIT( MI_ADDIU( _sp, _sp, -stack_frame_size( nr_locals ) ) );

	// load constants
	const_emit_loadreg( me );
}

void emit_epilogue( mips_emitter* me, int nr_locals ){
	// remember epilogue start
	me->epi = GET_EC( me );

	// move to prior stack frame
	EMIT( MI_ADDIU( _sp, _sp, stack_frame_size( nr_locals ) ) );
	EMIT( MI_LW( _ra, _sp, -4 ) );
	EMIT( MI_LW( _a1, _sp, -12 ) );		// base result virtual register
	EMIT( MI_LW( _a3, _sp, -16 ) );		// number of results 

	// TODO: write the results to the location 


	// jump back
	EMIT( MI_JR( _ra ) );
	EMIT( MI_NOP() );
}


