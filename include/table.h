/*
* (C) Iain Fraser - GPLv3
*/ 

#ifndef _TABLE_H_
#define _TABLE_H_

#include "lobject.h"

struct table* table_create( int array, int hash );
void table_setlist( struct table* t, void* src, int idx, int sz );


void table_set( struct table* t, struct TValue idx, struct TValue v );
struct TValue table_get( struct table* t, struct TValue idx );

#endif
