/*
* (C) Iain Fraser - GPLv3  
* Physical machine interface. Each arch supported needs to implement it.
*/

#ifndef _MACHINE_H_
#define _MACHINE_H_

#include <assert.h>	// TODO: remove once error checking in temp_reg
#include "operand.h"
#include "emitter.h"

#ifdef _ELFDUMP_
#include "elf.h"
#endif

struct JFunc;

struct machine {
	int 		sp,fp,ra;	// stack pointer, frame pointer and return address reg 
	bool		is_ra;		// is return address register ( if not assume call includes push )
	int 		nr_reg;
	int 		nr_temp_regs;  	// first NR_TEMP_REGS registers are temps the rest are used for locals and consts
	bool		allow_spill;	// allow acquire_register to spill temp onto the stack  
	int		max_access;	// max number of temps accessed, just by JITFunc 

#ifdef _ELFDUMP_
	Elf32_Half	elf_machine;
	unsigned char	elf_endian;	
#endif

	uint32_t 	reg[];		// upper 2 words are free for emitter use for temp allocation
};

/*
* Generic integer machine. Assume 2's complement and overflows are ignored. Also
* division truncates the quotient. So the following can be deduced:
* - Signed and unsigned add, sub, multiply ( therefore pow ) are equivalent.  
* - Branch compare instructions rely on add,sub and therefore are signed/unsigned indifferent.
* - Mod is trunc based ergo it shares the sign of the dividend.
*/
struct machine_ops {
	void (*move)( struct emitter* me, struct machine* m, operand d, operand s );

	/*
	* Compulsory arithmetic. 
	*/
	void (*add)( struct emitter* me, struct machine* m, operand d, operand s, operand t );
	void (*sub)( struct emitter* me, struct machine* m, operand d, operand s, operand t );
	void (*mul)( struct emitter* me, struct machine* m, operand d, operand s, operand t );
	void (*sdiv)( struct emitter* me, struct machine* m, operand d, operand s, operand t );
	void (*udiv)( struct emitter* me, struct machine* m, operand d, operand s, operand t );
	void (*smod)( struct emitter* me, struct machine* m, operand d, operand s, operand t );
	void (*umod)( struct emitter* me, struct machine* m, operand d, operand s, operand t );
	void (*pow)( struct emitter* me, struct machine* m, operand d, operand s, operand t );

	/*
	* Compulsory branching instructions. All branches expect temp registers to be
	* UNclobbered regardless of whether the branch is taken or not.  
	*/
	void (*b)( struct emitter* me, struct machine* m, label l );
	void (*beq)( struct emitter* me, struct machine* m, operand d, operand s, label l );
	void (*blt)( struct emitter* me, struct machine* m, operand d, operand s, label l );
	void (*bgt)( struct emitter* me, struct machine* m, operand d, operand s, label l );
	void (*ble)( struct emitter* me, struct machine* m, operand d, operand s, label l );
	void (*bge)( struct emitter* me, struct machine* m, operand d, operand s, label l );

	/*
	* Compulsory function calling instructions. Call is allowed to clobber temps before call. 
	* But after call they must be restored as usual. Also call is expected to push current
	* return address on stack before call.  
	*/
	void (*call)( struct emitter* me, struct machine* m, label l );
	void (*ret)( struct emitter* me, struct machine* m );

	/*
	* Calling C functions. Two ways:
	*	- Static, used for calling internal functions e.g. gettable
	*	- Dynamic, call embedded C functions from Lua vm.i
	*
	* Can clober any temp regs. Do not expect temp regs to be restored after
	* call.
	*/
	void (*call_static_cfn)( struct emitter* me, struct frame* f, uintptr_t fn, const operand* r, size_t argsz, ... );
	
	// optional instructions  
	void (*push)( struct emitter* me, struct machine* m, operand s );
	void (*pop)( struct emitter* me, struct machine* m, operand d );


	/*
	* generate arch jit functions see jitfunc.c 
	*/
	int (*nr_jfuncs)();
	void (*jf_init)( struct JFunc* jf, struct emitter* me, struct machine* m, int idx );

	// each machine has an associated emitter 
	void (*create_emitter)( struct emitter** e, size_t vmlines, e_realloc era );
};

/*
* Temporary management for (sub)instructions. 
*/


int temps_accessed( struct machine* m );
int acquire_temp( struct machine_ops* mop, struct emitter* e, struct machine* m );
bool release_temp( struct machine_ops* mop, struct emitter* e, struct machine* m );
void release_tempn( struct machine_ops* mop, struct emitter* e, struct machine* m, int n );
bool is_temp( struct machine* m, int reg );

bool disable_spill( struct machine* m );
void restore_spill( struct machine* m, bool prior );
void enable_spill( struct machine* m );

#define has_spill( m )	( temps_accessed( m ) > m->nr_temp_regs )


/*
* Load operands into simultaneous registers ( using temporaries ). 
*/

int load_coregisters( struct machine_ops* mop, struct emitter* me, struct machine* m, int nr_oper, ... );
void unload_coregisters( struct machine_ops* mop, struct emitter* me, struct machine* m, int n );

#endif 

