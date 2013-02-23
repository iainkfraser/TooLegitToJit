/*
* (C) Iain Fraser - GPLv3  
* Physical machine interface. Each arch supported needs to implement it.
*/

#ifndef _MACHINE_H_
#define _MACHINE_H_

void arch_move( struct arch_emitter* me, operand d, operand s );
void arch_add( struct arch_emitter* me, operand d, operand s, operand t );
void arch_sub( struct arch_emitter* me, operand d, operand s, operand t );
void arch_mul( struct arch_emitter* me, operand d, operand s, operand t );
void arch_div( struct arch_emitter* me, operand d, operand s, operand t );
void arch_mod( struct arch_emitter* me, operand d, operand s, operand t );
void arch_pow( struct arch_emitter* me, operand d, operand s, operand t );


void arch_call_cfn( struct arch_emitter* me, uintptr_t fn, size_t argsz );

#endif 

