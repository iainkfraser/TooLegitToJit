/*
* (C) Iain Fraser - GPLv3 
* Lua state.
*/

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "lstate.h"

static lua_State* current;		// TODO: this is a temp solution change it later!! 

void lstate_preinit( lua_State* ls ){
	INIT_LIST_HEAD( &ls->openuvs );

	current  = ls;
}

lua_State* current_state(){
	return current;	
}
