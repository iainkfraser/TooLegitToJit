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
#ifdef _ELFDUMP_
	int	strtabidx;	// string table index
#endif
};

enum { JF_ARG_RES_CPY, JF_COUNT };

struct emitter* jfuncs_init( struct machine_ops* mop, struct machine* m );
void jfuncs_setsection( void* section );
void jfuncs_cleanup( );
struct JFunc* jfuncs_get( int idx );

#define JFUNC_UNLIMITED_STACK	9999	
void jfunc_call( struct machine_ops* mop, struct emitter* e, struct machine* m, int idx, int maxstack, int nargs, ... );

// callback for creating JIT functions
typedef void (*jf_init)( struct JFunc* jf, struct machine_ops* mop, struct emitter* e, struct machine* m );

#endif
