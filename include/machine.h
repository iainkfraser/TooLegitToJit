/*
* (C) Iain Fraser - GPLv3  
* Physical machine interface. Each arch supported needs to implement it.
*/

#ifndef _MACHINE_H_
#define _MACHINE_H_

#include "operand.h"
#include "emitter.h"

struct machine {
	int sp;
	int nr_reg;
	int reg[];
};

struct machine_ops {
	void (*move)( struct emitter* me, struct machine* m, operand d, operand s );
	void (*add)( struct emitter* me, struct machine* m, operand d, operand s, operand t );
	void (*sub)( struct emitter* me, struct machine* m, operand d, operand s, operand t );
	void (*mul)( struct emitter* me, struct machine* m, operand d, operand s, operand t );
	void (*div)( struct emitter* me, struct machine* m, operand d, operand s, operand t );
	void (*mod)( struct emitter* me, struct machine* m, operand d, operand s, operand t );
	void (*pow)( struct emitter* me, struct machine* m, operand d, operand s, operand t );
	void (*call_cfn)( struct emitter* me, struct machine* m, uintptr_t fn, size_t argsz );

	// each machine has an associated emitter 
	void (*create_emitter)( struct emitter** e, size_t vmlines );
};

#endif 

