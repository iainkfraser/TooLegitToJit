/*
* (C) Iain Fraser - GPLv3
*/ 

#ifndef _TABLE_H_
#define _TABLE_H_

struct table;

struct table* table_create( int array, int hash );
void table_set( struct table* t, int idx, int type, int value );
int table_get( struct table* t, int idx, int* type );

#endif
