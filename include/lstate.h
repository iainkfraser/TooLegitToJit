/*
* (C) Iain Fraser - GPLv3 
* Lua state.
*
* Idea, If memory is an issue the virtual stack could be growth could
* be adaptive by using a table.
*/

#ifndef _LSTATE_H_
#define _LSTATE_H_

#include "list.h"
#include "lobject.h"

#define MAX_STACK	32

struct lua_State {
	// could put garbage collection here

	struct list_head openuvs;		// open upvals list	
	struct TValue genv;			// TODO: make it the registry 
	struct TValue stack[ MAX_STACK ];	// vstack communicate with C
	struct TValue* top;			// top of the stack
};


void lstate_preinit( lua_State* ls );
lua_State* current_state();	// the current execution context lua state
#endif
