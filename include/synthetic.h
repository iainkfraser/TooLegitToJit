/*
* (C) Iain Fraser - GPLv3  
* Sythentic generic machine instructions.
*/

#ifndef _SYNTHETIC_H_
#define _SYNTHETIC_H_

#include "machine.h"
#include "emitter.h"

void assign( struct machine_ops* mop, struct emitter* e, struct machine* m, vreg_operand d, vreg_operand s );
void loadim( struct machine_ops* mop, struct emitter* e, struct machine* m, int reg, int value ); 

#endif
