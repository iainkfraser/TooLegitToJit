/*
* (C) Iain Fraser - GPLv3 
* Lua state.
*/

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "lstate.h"
#include "table.h"
#include "frame.h"
#include "macros.h"

static lua_State* current;		// TODO: this is a temp solution change it later!! 

void lstate_preinit( lua_State* ls, jit_bootstrap jbs ){
	INIT_LIST_HEAD( &ls->openuvs );

	ls->jbs = jbs;
	
	// setup prefunction stack
	initstack( ls, ls->gstack, array_count( ls->gstack ) );

	// setup global environment 
	ls->genv.t = ctb( LUA_TTABLE ); 
	ls->genv.v.gc = (struct gcheader*)table_create( 0, 0 ); 


	current  = ls;
}

lua_State* current_state(){
	return current;	
}
