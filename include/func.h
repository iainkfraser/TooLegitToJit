/*
* (C) Iain Fraser - GPLv3 
*/

#ifndef _FUNC_H_
#define _FUNC_H_

#include "lobject.h"

struct closure* closure_create( struct proto* p, struct closure* parent, struct TValue* stackbase );

#endif

