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
#include "macros.h"

/*
** assertion for checking API calls
*/
#if !defined(luai_apicheck)
#define luai_apicheck(L,e)	assert(e)
#else
#define luai_apicheck(L,e)	((void)0)
#endif

void initstack( lua_State *L, struct TValue* s, size_t sz ){
	L->stack = s;
	L->top = L->stack + sz;
}

static struct TValue* stk_base( lua_State * L ){
	return L->stack + LUA_MINSTACK; //array_count( L->stack );
}

static struct TValue* stk_entry( lua_State* L, int idx ){
	return stk_base( L ) - idx;
}

static int stk_idx( lua_State* L, struct TValue* entry ){
	return stk_base( L ) - entry;
}

// TODO: stack manipulation
struct TValue* index2addr (lua_State *L, int idx) {
	int top = lua_gettop( L );
		
	if( idx > 0 ){
		luai_apicheck( L, idx <= top );
		return stk_entry( L, idx );	
	} else if ( idx > LUA_REGISTRYINDEX ){
		luai_apicheck( L, idx != 0 && -idx <= top );
		return stk_entry( L, ( top + idx ) + 1 );
	} else if ( idx == LUA_REGISTRYINDEX ) {
		assert( false );	// access the registry 
	} else {
		assert( false );	// TODO: upvalues??
	}
	return NULL;
}

void lua_safepush( lua_State *L, struct TValue v ){
	luai_apicheck( L, L->top > L->stack );
	*--L->top = v;
} 

struct TValue lua_safepop( lua_State *L ){
	return *L->top++; 
}

/*
** basic stack manipulation
*/


int lua_gettop(lua_State *L){
	return stk_idx( L, L->top );
}	

/*
** push functions (C -> stack)
*/

void lua_pushnumber(lua_State *L, lua_Number n){
	struct TValue v = {
		.t = LUA_TNUMBER,
		.v.n = n
	};
	
	lua_safepush( L, v );
}

void lua_pushcclosure(lua_State *L, lua_CFunction fn, int n) {
	assert( n == 0 );	// No C closures atm 
	struct TValue v = {
		.t = LUA_TLCF,
		.v.f = fn 
	};

	lua_safepush( L, v );
}



/*
** set functions (stack -> Lua)
*/

void lua_setglobal(lua_State *L, const char *var){
	struct TValue key = { 
		.t = LUA_TSTRING, 
		.v.gc = ( struct gcheader*) var 
	};
	
	table_set( (struct table*) L->genv.v.gc, key, lua_safepop( L ) );
}

/*
** get functions (Lua -> stack)
*/
void lua_getglobal(lua_State *L, const char *var){
	struct TValue key = { 
		.t = LUA_TSTRING, 
		.v.gc = ( struct gcheader*) var 
	};
	
	struct TValue g = table_get( (struct table*) L->genv.v.gc, key );
	lua_safepush( L, g );
}

/*
** 'load' and 'call' functions (load and run Lua code)
*/

void lua_call (lua_State *L, int nargs, int nresults){
	struct TValue* closure = index2addr( L, 1 );

	int n = L->jbs( nargs, &closure->v );

	// TODO: copy the results 
}

int lua_pcall (lua_State *L, int nargs, int nresults, int msgh){
	return 0;
}
