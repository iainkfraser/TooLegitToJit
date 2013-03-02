/*
* (C) Iain Fraser - GPLv3  
* JIT Functions are functions used exclusively by JIT'r. 
*/

#ifndef _JITFUNC_H_
#define _JITFUNC_H_

#include "operand.h"
#include "machine.h"

struct JFunc {
	void*		addr;
	int 		nr_args;
	int		nr_clobber;
	operand*	params;		// argument locations
	int*		cregs;		// registers clobbered by function
	int		cstack;		// amount of stack clobbered 
};

enum { JF_MEMCPY, JF_COUNT };

struct emitter* jfuncs_create( struct machine* m, struct machine_ops* mop );
void jfuncs_cleanup( );
struct JFunc* jfuncs_get( int idx );

#endif
