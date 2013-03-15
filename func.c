/*
* (C) Iain Fraser - GPLv3 
*/

#include <stdio.h>
#include <stdint.h>
#include "func.h"


struct closure* closure_create( struct proto* addr, struct closure* c ){
	printf("create closure %p\n", addr->code );
}

