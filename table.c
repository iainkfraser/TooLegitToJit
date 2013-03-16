/*
* (C) Iain Fraser - GPLv3
*/ 

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include <errno.h>
#include "table.h"
#include "frame.h"
#include "lopcodes.h"
#include "lobject.h"

#define THROW( e )	do{}while(0)	


typedef struct table {
	int 	ref;
	void*	hash;
	int	array[];
} table_t; 

table_t* table_create( int array, int hash ){
	size_t sz = sizeof( table_t ) + sizeof( int ) * array;
	table_t* t = malloc( sizeof( sz ) );
	if( !t )
		THROW( ENOMEM );
	t->ref = 0;
	printf("create table %p\n", t );

	return t;
}


void table_setlist( struct table* t, void* src, int idx, int sz ){
	for( int i = 0; i < sz; i++ ){
	//	t->array[ idx + i ] = (char*)src + vreg_value_offset( i )
		memcpy( &t->array[ idx + i ],
			(char*)src + vreg_value_offset( i ),
			sizeof( int ) );	// TODO: how do you know size? Need word size
	}
}

void table_set( struct table* t, struct TValue idx, struct TValue v ){
	if( idx.t == LUA_TSTRING )
		return;
	t->array[ idx.v.n ] = v.v.n;
}

struct TValue table_get( struct table* t, struct TValue idx ){
	if( idx.t == LUA_TSTRING ){
		struct TValue r = { .t = LUA_TNUMBER, .v.n = 70 };
		return r;
	}

	struct TValue ret;
	ret.v.n = t->array[ idx.v.n - 1 ];
	ret.t = LUA_TNUMBER;
	return ret;
}

/*
* Lua jit callee functions ( args and return must be word/wordp ).
*/

wordp ljc_tableget( wordp t, word idxt, word idxv, wordp type ){
	Tag* tag = (Tag*)type;
	struct TValue idx = { .t = idxt, .v.n = idxv };	
	struct TValue v = table_get( (struct table*)t, idx );
	*tag = v.t;
	return v.v.n;
}

void ljc_tableset( wordp t, word idxt, word idxv, word valt, word valv ){
	struct TValue idx = { .t = idxt, .v.n = idxv };	
	struct TValue v = { .t = valt, .v.n = valv };
	table_set( ( struct table*)t, idx, v );
}

