/*
* (C) Iain Fraser GPLv3
*
* Dictionary interface used by table.
*/

#ifndef _DICTIONARY_H_
#define _DICTIONARY_H_

#include "lobject.h"

struct dictionary;

struct dictionary* dictionary_create( int nr_slots );
void dictionary_destroy( struct dictionary *d );
bool dictionary_delete( struct dictionary *d, struct TValue k );
struct TValue dictionary_search( struct dictionary *d, struct TValue k );	
bool dictionary_insert( struct dictionary *d, struct TValue k, 
					struct TValue v );
#endif
