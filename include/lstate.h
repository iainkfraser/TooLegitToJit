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

typedef int (*jit_bootstrap)( int nr_args, union Value* closure );

struct lua_State {
	// could put garbage collection here

	struct list_head openuvs;		// open upvals list	
	struct TValue genv;			// TODO: make it the registry 
	struct TValue gstack[ LUA_MINSTACK ];	// vstack communicate with C
	struct TValue* stack;
	struct TValue* top;			// top of the stack
	jit_bootstrap jbs;
};



void lstate_preinit( lua_State* ls, jit_bootstrap jbs );
lua_State* current_state();	// the current execution context lua state
#endif
