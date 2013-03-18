/*
* (C) Not Me - GPLv3
* Temp shitty linked list implementation of dictionary.
*/

#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <stddef.h>
#include "list.h"
#include "lopcodes.h"
#include "lobject.h"

struct dictionary {
	struct list_head head;	
};

struct node {
	struct TValue k,v;
	struct list_head link;
};


struct dictionary* dictionary_create( int nr_slots ){
	struct dictionary* d = malloc( sizeof( struct dictionary ) );
	assert( d );
	INIT_LIST_HEAD( &d->head );
	return d;
}

void dictionary_destroy( struct dictionary *d ){
	// REMOVE ALL LIST
	free( d );
}

static struct node* find( struct dictionary * d, struct TValue k ){
	struct list_head *seek; 
	list_for_each( seek, &d->head ){
		struct node* n = list_entry( seek, struct node, link );	
		if( n->k.t == k.t && n->k.v.n == k.v.n )
			return n;
	}

	return NULL;
}

bool dictionary_delete( struct dictionary *d, struct TValue k ){
	struct node* n = find( d, k );
	if( n ){
		list_del( &n->link );
		free( n );
		return true;
	}

	return false;
}

struct TValue dictionary_search( struct dictionary *d, struct TValue k ){
	// TODO: bunch of operations aren't setting type
	if( k.t != LUA_TSTRING )
		k.t = LUA_TNUMBER;
	
	struct TValue r = { .t = LUA_TNIL };
	struct node* n = find( d, k );
	return n ? n->v : r;
		
}

bool dictionary_insert( struct dictionary *d, struct TValue k, 
					struct TValue v ){
	bool found;
	struct node* n = find( d, k );
	if( !( found = !!n ) ){
		n = malloc( sizeof( struct node ) );
		assert( n );
		list_add( &n->link, &d->head );	
		n->k = k;
	}

	n->v = v;
	return found;
}

