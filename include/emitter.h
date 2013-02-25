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

struct emitter_ops {
	size_t (*link)( struct emitter* e );
	void* (*stop)( struct emitter* e, void* buf, unsigned int startec );

	void (*label_pc)( struct emitter* e, unsigned int pc );
	void (*label_local)( struct emitter* e, unsigned int local );

	void (*branch_pc)( struct emitter* me, int line );
	void (*branch_local)( struct emitter* me, int local, bool next );

	unsigned int (*ec)(struct emitter* e );
};

struct emitter {
	struct emitter_ops*	ops;
};

#endif
