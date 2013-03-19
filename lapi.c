/*
* (C) Iain Fraser - GPLv3 
* Lua C API.
*/

#include <assert.h>
#include <stddef.h>
#include <stdbool.h>
#include "lapi.h"
#include "lobject.h"
#include "lstate.h"
#include "table.h"

// TODO: stack manipulation


/*
** push functions (C -> stack)
*/

void lua_pushcclosure(lua_State *L, lua_CFunction fn, int n) {
	assert( n == 0 );	// No C closures atm 
	L->top->t = LUA_TLCF;
	L->top->v.f = fn;
	L->top++;
}



/*
** set functions (stack -> Lua)
*/

void lua_setglobal(lua_State *L, const char *var){
	struct TValue key = { 
		.t = LUA_TSTRING, 
		.v.gc = ( struct gcheader*) var 
	};
	
	table_set( (struct table*) L->genv.v.gc, key, *(--L->top) );
}

