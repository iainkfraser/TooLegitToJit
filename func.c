/*
* (C) Iain Fraser - GPLv3 
*/

#include <stdio.h>
#include <stdint.h>
#include "func.h"

void closure_create( struct proto* addr, struct closure* c ){
	printf("create closure %x\n", (uintptr_t)addr->code );
}

