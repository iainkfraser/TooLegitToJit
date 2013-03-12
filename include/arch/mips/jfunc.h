/*
* (C) Iain Fraser - GPLv3  
* MIPS jit functions.
*/

#ifndef _ARCH_MIPS_JFUNC_H_
#define _ARCH_MIPS_JFUNC_H_

enum { MJF_SAVENCALL, MJF_COUNT };

int mips_nr_jfuncs();
void mips_jf_init( struct JFunc* jf, struct emitter* me, struct machine* m, int idx );

#endif
