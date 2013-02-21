/*
* (C) Iain Fraser - GPLv3 
*/

#ifndef _XLOGUE_H_
#define _XLOGUE_H_

void emit_epilogue( mips_emitter* me, int nr_locals );
void emit_prologue( mips_emitter* me, int nr_locals, int nr_params );

#endif

