/*
* (C) Iain Fraser - GPLv3 
*
* Lua string implementation.
*/

#include <assert.h>
#include <string.h>
#include "lstring.h"


int lstrcmp( struct TValue* s, struct TValue* t ){
	assert( tag( s ) == LUA_TSTRING && tag( t ) == LUA_TSTRING );
	return strcmp( tvtostr( s ), tvtostr( t ) );
}
