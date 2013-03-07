/*
* (C) Iain Fraser - GPLv3  
* MIPS branch instructions implementation. 
*/

#ifndef _MIPS_BRANCH_H_
#define _MIPS_BRANCH_H_


void mips_b( struct emitter* me, struct machine* m, label l );
void mips_beq( struct emitter* me, struct machine* m, operand d, operand s, label l );
void mips_blt( struct emitter* me, struct machine* m, operand d, operand s, label l );
void mips_bgt( struct emitter* me, struct machine* m, operand d, operand s, label l );
void mips_ble( struct emitter* me, struct machine* m, operand d, operand s, label l );
void mips_bge( struct emitter* me, struct machine* m, operand d, operand s, label l );

#endif
