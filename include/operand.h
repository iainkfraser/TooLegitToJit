/*
* (C) Iain Fraser - GPLv3 
*
* Platfrom indepdent vm and physical machine operand mapping.
*/

#ifndef _OPERAND_H_
#define _OPERAND_H_

#include "mc_emitter.h"

vreg_operand llocal_to_stack_operand( struct arch_emitter* me, int vreg );
vreg_operand llocal_to_operand( struct arch_emitter* me, int vreg );
vreg_operand lconst_to_operand( struct arch_emitter* me, int k );
vreg_operand loperand_to_operand( struct arch_emitter* me, loperand op );


#endif
