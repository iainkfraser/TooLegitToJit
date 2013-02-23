/*
* (C) Iain Fraser - GPLv3 
*/

#ifndef _FUNC_H_
#define _FUNC_H_

#include "instruction.h"

struct closure;
struct closure* closure_create( struct proto* addr, struct closure* c );

#endif

