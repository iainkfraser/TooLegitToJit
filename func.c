/*
* (C) Iain Fraser - GPLv3 
*/

#include <stdio.h>
#include <stdint.h>
#include "func.h"

struct closure {
	struct proto* p;
	uint32_t upval[];	
};

struct closure* closure_create( struct proto* addr, struct closure* c ){
	printf("create closure %x\n", (uintptr_t)addr->code );
}

