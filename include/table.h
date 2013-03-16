/*
* (C) Iain Fraser - GPLv3
*/ 

#ifndef _TABLE_H_
#define _TABLE_H_

#include "lobject.h"

struct table;

struct table* table_create( int array, int hash );
void table_set( struct table* t, int idx, int type, int value );
void table_setlist( struct table* t, void* src, int idx, int sz );

struct TValue table_get( struct table* t, struct TValue idx );

/*
* Lua jitter callee funcs
*/

wordp ljc_tableget( wordp t, word idxt, word idxv, wordp type );

#endif
