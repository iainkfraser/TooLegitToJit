/*
* (C) Iain Fraser - GPLv3 
*/

#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "frame.h"
#include "lstate.h"
#include "func.h"


struct UpVal* find_stackupval( lua_State* ls, struct TValue* stackbase, int idx ){
	struct list_head *seek;
	struct UpVal* uv;
	struct TValue* dst = vreg_tvalue_offset( stackbase, idx );

	list_for_each( seek, &ls->openuvs ){
		uv = list_entry( seek, struct UpVal, link );	
		if( uv->val == dst )
			return uv;		
	}

	// create new upval
	uv = malloc( sizeof( struct UpVal ) );
	if( !uv )
		assert( false );		// TODO: jump with out of memory exception

	uv->val = dst;
	list_add( &uv->link, &ls->openuvs );
	return uv;	
}

/*
* Migrate all openvalues that beyond(below) the stackbase. Called when
* function is returning. 
*/ 
void closure_close( struct TValue* stackbase ){
	lua_State* ls = current_state();
	struct UpVal* uv;
	struct list_head *seek,*safe;
	list_for_each_safe( seek, safe, &ls->openuvs ){
		uv = list_entry( seek, struct UpVal, link );	
	
		assert( uv->val != &uv->v );	// all openvalues are on the stack

		// assume stack grows downwards 
		if( uv->val > stackbase )
			break; 

		// migrate
		list_del( seek ); 
		uv->v = *uv->val;
		uv->val = &uv->v;

		// TODO: call garbage collect? 
	}  
}

/*
* Close environment by linking all upvalues. 
*/
struct closure* closure_create( struct proto* p, struct closure** pparent, struct TValue* stackbase ){
	#define parent	( *pparent )	// pparent maybe NULL e.g. main chunk so do lazy deref

	size_t sz = sizeof( struct closure ) + sizeof( struct UpVal* ) * p->sizeupvalues;
	struct closure* c = malloc( sz );
	if( !c )
		assert( false );		// TODO: jump with out of memory exception

	for( int i = 0; i < p->sizeupvalues; i++ ){
		if( p->uvd[i].instack )
			c->uvs[i] = find_stackupval( current_state(), stackbase, p->uvd[i].idx );	
		else 
			c->uvs[i] = parent->uvs[ p->uvd[i].idx ];	
		
	}

	c->p = p;
	return c;
}


