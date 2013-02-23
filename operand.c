/*
* (C) Iain Fraser - GPLv3 
*
* Platfrom indepdent vm and physical machine operand mapping.
*/

#include "instruction.h"
#include "operand.h"


vreg_operand llocal_to_stack_operand( struct arch_emitter* me, int vreg ){
	return arch_vreg_to_operand( arch_nr_locals( me ), vreg, true ); 
}

vreg_operand llocal_to_operand( struct arch_emitter* me, int vreg ){
	return arch_vreg_to_operand( arch_nr_locals( me ), vreg, false ); 
}

vreg_operand lconst_to_operand( struct arch_emitter* me, int k ){
	return arch_const_to_operand( me, k );
}

vreg_operand loperand_to_operand( struct arch_emitter* me, loperand op ){
	return op.islocal ? 
		llocal_to_operand( me, op.index ) :
		lconst_to_operand( me, op.index );
}
