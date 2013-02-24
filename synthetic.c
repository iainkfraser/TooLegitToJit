/*
* (C) Iain Fraser - GPLv3  
* Sythentic generic machine instructions.
*/

#include "mc_emitter.h"
#include "machine.h"

void assign( struct arch_emitter* me, vreg_operand d, vreg_operand s ){
	arch_move( me, d.value, s.value );
	arch_move( me, d.type, s.type );
}


void loadim( struct arch_emitter* me, int reg, int value ){
	operand r = OP_TARGETREG( reg );
	operand v = OP_TARGETIMMED( value );
	arch_move( me, r, v );
}

