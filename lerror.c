/*
* (C) Iain Fraser - GPLv3 
*
* Error handling.
*/

#include <stdio.h>
#include "lstate.h"
#include "lobject.h"
#include "lerror.h"


void lerror( lua_State* L, int ecode ){
	printf("ERROR!");
}

void lcurrent_error( int ecode ){
	lerror( current_state(), ecode );
}
