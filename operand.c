/*
* (C) Iain Fraser - GPLv3 
*
* Platfrom indepdent vm and physical machine operand mapping.
*/

#include "instruction.h"
#include "operand.h"
#include "lopcodes.h"

vreg_operand llocal_to_stack_operand( struct frame* f, int vreg ){
	return vreg_to_operand( f, vreg, true ); 
}

vreg_operand llocal_to_operand( struct frame* f, int vreg ){
	return vreg_to_operand( f, vreg, false ); 
}

vreg_operand lconst_to_operand( struct frame* f, int k ){
	return const_to_operand( f, k );
}

vreg_operand loperand_to_operand( struct frame* f, loperand op ){
	return op.islocal ? 
		llocal_to_operand( f, op.index ) :
		lconst_to_operand( f, op.index );
}


loperand to_loperand( int rk ){
	loperand r;
	r.islocal = !ISK( rk );
	r.index = r.islocal ? rk : INDEXK( rk );  
	return r;
} 

