/*
* (C) Iain Fraser - GPLv3  
* MIPS jit functions.
*/

#ifndef _ARCH_MIPS_JFUNC_H_
#define _ARCH_MIPS_JFUNC_H_

enum { MJF_STORETEMP, MJF_LOADTEMP, MJF_COUNT };

int mips_nr_jfuncs();
void mips_jf_init( struct JFunc* jf, struct emitter* me, struct machine* m, int idx );

void mips_jf_storetemp( struct JFunc* jf, struct emitter* me, struct machine* m );
void mips_jf_loadtemp( struct JFunc* jf, struct emitter* me, struct machine* m );


int mjf_storetemp_offset( struct machine* m, int nr_locals );
int mjf_loadtemp_offset( struct machine* m, int nr_locals );

#endif
