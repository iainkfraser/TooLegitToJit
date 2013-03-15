/*
* (C) Iain Fraser - GPLv3 
* Lua state.
*/

#ifndef _LSTATE_H_
#define _LSTATE_H_

#include "lobject.h"

typedef struct lua_State {
	// could put garbage collection here

	struct list_head openuvs;	// open upvals list	
} lua_State;


void lstate_preinit( lua_State* ls );
lua_State* current_state();	// the current execution context lua state

#endif
