/*
* (C) Iain Fraser - GPLv3 
*/

#ifndef _FUNC_H_
#define _FUNC_H_

#include "mc_emitter.h"

struct closure {
	struct proto* p;
	uint32_t upval[];	
};

void closure_create( struct proto* addr, struct closure* c );

#endif

