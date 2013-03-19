/*
* (C) Iain Fraser - GPLv3
* 
* Note on Lua 5.x table implementation. Lua has the array/table
* hybrid. Few points:
*	1) Array value n ( based on invariants ) is only recalculated
*	   when the hash table is forced to resize  i.e. the hash table has 
*          been exhausted ( see point 2 ).
*	2) The hash table is implemented as a variant of chained scatter
*	   table. Kinda like open addressing except each probe is done
*	   by pointer in collided slot to next available slot. Since its
*	   not chained the table is exhausted when every slot is occupied.
* 
* This means we either have some interface for handling exhaustation so
* that the array operations can be generic. Or just leave the array integration
* to each speclisation. For now ill just have pure dictionary ( no array )
* with trivial linked list implementation. Till performance demands dictate 
* the direction.
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
#include "dictionary.h"

#define THROW( e )	do{}while(0)	

table_t* table_create( int array, int hash ){
	array = 0;	// for now 

	size_t sz = sizeof( table_t ) + sizeof( struct TValue ) * array;
	table_t* t = malloc( sizeof( sz ) );
	if( !t )
		THROW( ENOMEM );

	t->asz = array;
	t->d = dictionary_create( hash );
	return t;
}


void table_setlist( struct table* t, void* src, int idx, int sz ){
	struct TValue *v, key = { .t = LUA_TNUMBER, .v.n = idx + 1 };

	for( int i = 0; i < sz; i++, key.v.n++ ){
		v = (struct TValue*)((char*)src + vreg_type_offset( i ));
		table_set( t, key, *v );
	}
#if 0
	for( int i = 0; i < sz; i++ ){
	//	t->array[ idx + i ] = (char*)src + vreg_value_offset( i )
		memcpy( &t->array[ idx + i ],
			(char*)src + vreg_value_offset( i ),
			sizeof( int ) );	// TODO: how do you know size? Need word size
	}
#endif
}

void table_set( struct table* t, struct TValue idx, struct TValue v ){
	dictionary_insert( t->d, idx, v );
}

struct TValue table_get( struct table* t, struct TValue idx ){
	return dictionary_search( t->d, idx );
}

/*
* Lua jit callee functions ( args and return must be lua_Number/LUA_PTR ).
*/

LUA_PTR ljc_tableget( LUA_PTR t, lua_Number idxt, lua_Number idxv
							, LUA_PTR type ){
	Tag* tag = (Tag*)type;
	struct TValue idx = { .t = idxt, .v.n = idxv };	
	struct TValue v = table_get( (struct table*)t, idx );
	*tag = v.t;
	return v.v.n;
}

void ljc_tableset( LUA_PTR t, lua_Number idxt, lua_Number idxv
							, lua_Number valt
							, lua_Number valv ){
	struct TValue idx = { .t = idxt, .v.n = idxv };	
	struct TValue v = { .t = valt, .v.n = valv };
	table_set( ( struct table*)t, idx, v );
}

