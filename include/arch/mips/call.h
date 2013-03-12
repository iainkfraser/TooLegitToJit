/*
* (C) Iain Fraser - GPLv3  
* MIPS call instructions implementation. 
*/

#ifndef _MIPS_CALL_H_
#define _MIPS_CALL_H_

void mips_call( struct emitter* me, struct machine* m, label l );
void mips_ret( struct emitter* me, struct machine* m );
void mips_static_ccall( struct emitter* me, struct frame* f, uintptr_t fn, const operand* r, size_t argsz, ... );

#endif
