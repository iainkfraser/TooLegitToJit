/*
* (C) Iain Fraser - GPLv3  
* Sythentic generic machine instructions.
*/

#include "emitter.h"
#include "machine.h"

void assign( struct machine_ops* mop, struct emitter* e, struct machine* m, vreg_operand d, vreg_operand s ) {
	mop->move( e, m, d.value, s.value );
	mop->move( e, m, d.type, s.type );
}


void loadim( struct machine_ops* mop, struct emitter* e, struct machine* m, int reg, int value ) {
	operand r = OP_TARGETREG( reg );
	operand v = OP_TARGETIMMED( value );
	mop->move( e, m, r, v );
}

