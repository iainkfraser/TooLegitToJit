/*
* (C) Iain Fraser - GPLv3  
* Physical machine interface. Each arch supported needs to implement it.
*/

#ifndef _MACHINE_H_
#define _MACHINE_H_

#include <assert.h>	// TODO: remove once error checking in temp_reg
#include "operand.h"
#include "emitter.h"

struct machine {
	int 		sp,fp;		// stack pointer and frame pointer 
	int 		nr_reg;
	int 		nr_temp_regs;  	// first NR_TEMP_REGS registers are temps the rest are used for locals and consts
	bool		allow_spill;	// allow acquire_register to spill temp onto the stack  
	int		max_access;	// max number of temps accessed, just by JITFunc 
	uint32_t 	reg[];		// upper 2 words are free for emitter use for temp allocation
};

struct machine_ops {
	void (*move)( struct emitter* me, struct machine* m, operand d, operand s );
	void (*add)( struct emitter* me, struct machine* m, operand d, operand s, operand t );
	void (*sub)( struct emitter* me, struct machine* m, operand d, operand s, operand t );
	void (*mul)( struct emitter* me, struct machine* m, operand d, operand s, operand t );
	void (*div)( struct emitter* me, struct machine* m, operand d, operand s, operand t );
	void (*mod)( struct emitter* me, struct machine* m, operand d, operand s, operand t );
	void (*pow)( struct emitter* me, struct machine* m, operand d, operand s, operand t );
	void (*b)( struct emitter* me, struct machine* m, label l );
	void (*beq)( struct emitter* me, struct machine* m, operand d, operand s, label l );
	void (*blt)( struct emitter* me, struct machine* m, operand d, operand s, label l );
	void (*bgt)( struct emitter* me, struct machine* m, operand d, operand s, label l );
	void (*ble)( struct emitter* me, struct machine* m, operand d, operand s, label l );
	void (*bge)( struct emitter* me, struct machine* m, operand d, operand s, label l );
	void (*call)( struct emitter* me, struct machine* m, operand fn );
	void (*ret)( struct emitter* me, struct machine* m );

	// optional instructions  
	void (*push)( struct emitter* me, struct machine* m, operand s );
	void (*pop)( struct emitter* me, struct machine* m, operand d );

	void (*call_cfn)( struct emitter* me, struct machine* m, uintptr_t fn, size_t argsz );

	// each machine has an associated emitter 
	void (*create_emitter)( struct emitter** e, size_t vmlines );
};

/*
* Temporary management for (sub)instructions. 
*/


int temps_accessed( struct machine* m );
int acquire_temp( struct machine_ops* mop, struct emitter* e, struct machine* m );
void release_temp( struct machine_ops* mop, struct emitter* e, struct machine* m );
void release_tempn( struct machine_ops* mop, struct emitter* e, struct machine* m, int n );
bool is_temp( struct machine* m, int reg );

bool disable_spill( struct machine* m );
void restore_spill( struct machine* m, bool prior );
void enable_spill( struct machine* m );


/*
* Load operands into simultaneous registers ( using temporaries ). 
*/

int load_coregisters( struct machine_ops* mop, struct emitter* me, struct machine* m, int nr_oper, ... );
void unload_coregisters( struct machine_ops* mop, struct emitter* me, struct machine* m, int n );

#endif 

