/*
* (C) Iain Fraser - GPLv3
*/ 

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include <errno.h>
#include "frame.h"

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


void table_set( struct table* t, int idx, int type, int value ){
	t->array[ idx - 1 ] = value;
}

int table_get( struct table* t, int idx, int* type ){
	return t->array[ idx-1 ];
}

void table_setlist( struct table* t, void* src, int idx, int sz ){
	for( int i = 0; i < sz; i++ ){
	//	t->array[ idx + i ] = (char*)src + vreg_value_offset( i )
		memcpy( &t->array[ idx + i ],
			(char*)src + vreg_value_offset( i ),
			sizeof( int ) );	// TODO: how do you know size? Need word size
	}
}

