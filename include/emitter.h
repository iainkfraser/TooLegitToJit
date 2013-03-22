/*
* (C) Iain Fraser - GPLv3  
* Pure machine code emitter interface.
*/

#ifndef _MC_EMITTER_H_
#define _MC_EMITTER_H_

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#define NR_LOCAL_LABELS		10

struct emitter;
typedef void* (*e_realloc)( void*, size_t, size_t );	// emitter reallactor 


struct emitter_ops {
	size_t (*link)( struct emitter* e );
	void* (*offset )( struct emitter* e, int offset, void* code );
	void (*cleanup)( struct emitter* e );

	void (*label_pc)( struct emitter* e, unsigned int pc );
	void (*label_local)( struct emitter* e, unsigned int local );

	void (*branch_pc)( struct emitter* me, int line );
	void (*branch_local)( struct emitter* me, int local, bool next );

	unsigned int (*pc)(struct emitter* e );
	unsigned int (*ec)(struct emitter* e );		// get current emitter address 
	uintptr_t (*absc)(struct emitter* e );		// get current absolute address
};

struct emitter {
	e_realloc		realloc;	
	struct emitter_ops*	ops;
};

#endif
