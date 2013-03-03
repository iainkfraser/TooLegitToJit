/*
* (C) Iain Fraser - GPLv3  
* JIT Functions are functions used exclusively by JIT'r. 
*/

#ifndef _JITFUNC_H_
#define _JITFUNC_H_

#include "operand.h"
#include "machine.h"

struct JFunc {
	uintptr_t	addr;
	int		temp_clobber;		// amount of stack clobbered 
};

enum { JF_ARG_RES_CPY, JF_COUNT };

struct emitter* jfuncs_init( struct machine_ops* mop, struct machine* m );
void jfuncs_setsection( void* section );
void jfuncs_cleanup( );

/*
* Get jfunc properties
*/
void* jfuncs_addr( struct machine* m, int idx );
int jfuncs_temp_clobber( struct machine* m, int idx );	// # of temp regs clobbered
int jfuncs_stack_clobber( struct machine* m, int idx );	// # of words on stack clobbered ( due to temp spill )

// callback for creating JIT functions
typedef void (*jf_init)( struct JFunc* jf, struct machine_ops* mop, struct emitter* e, struct machine* m );

#endif
