/*
* (C) Iain Fraser - GPLv3  
* Sythentic generic machine instructions.
*/

#ifndef _SYNTHETIC_H_
#define _SYNTHETIC_H_

void assign( struct arch_emitter* me, vreg_operand d, vreg_operand s );
void loadim( struct arch_emitter* me, int reg, int value ); 

#endif
