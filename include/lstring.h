/*
* (C) Iain Fraser - GPLv3 
*
* String interface. Allows for opaque internal implementation.
*/

#ifndef _LSTRING_H_
#define _LSTRING_H_

#include "lobject.h"

int lstrcmp( struct TValue* s, struct TValue* t );
bool lstrtonum( struct TValue* s, lua_Number* r );

#endif
