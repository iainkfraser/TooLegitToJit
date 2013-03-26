/*
* (C) Iain Fraser - GPLv3  
* MIPS arithmetic instructions implementation. 
*/

#ifndef _MIPS_ARITHMETIC_H_
#define _MIPS_ARITHMETIC_H_

void mips_add( struct emitter* me, struct machine* m, operand d, operand s, operand t );
void mips_sub( struct emitter* me, struct machine* m, operand d, operand s, operand t );
void mips_mul( struct emitter* me, struct machine* m, operand d, operand s, operand t );
void mips_pow( struct emitter* me, struct machine* m, operand d, operand s, operand t );
void mips_sdiv( struct emitter* me, struct machine* m, operand d, operand s, operand t );
void mips_udiv( struct emitter* me, struct machine* m, operand d, operand s, operand t );
void mips_smod( struct emitter* me, struct machine* m, operand d, operand s, operand t );
void mips_umod( struct emitter* me, struct machine* m, operand d, operand s, operand t );
void mips_or( struct emitter* me, struct machine* m, operand d, operand s, operand t );

#endif
