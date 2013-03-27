/*
* (C) Iain Fraser - GPLv3 
*
* Lua string implementation.
*/

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "lstring.h"


int lstrcmp( struct TValue* s, struct TValue* t ){
	assert( tag( s ) == LUA_TSTRING && tag( t ) == LUA_TSTRING );
	return strcmp( tvtostr( s ), tvtostr( t ) );
}

bool lstrtonum( struct TValue* s, lua_Number* r ){
	assert( tag( s ) == LUA_TSTRING );
	char *end,*str = tvtostr( s );

	if( strpbrk( str, "nN" ) )
		return false;
	else if( strpbrk( str, "xX" ) )
		*r = (lua_Number)strtol( str, &end, 16 );
	else
		*r = (lua_Number)strtol( str, &end, 10 );

	if( end == str )
		return false;

	return true;
}
