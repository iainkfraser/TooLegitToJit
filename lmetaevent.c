/*
* (C) Iain Fraser - GPLv3 
*
* Handle events using values metatable as described in reference man.
*/

#include <stddef.h>
#include "lobject.h"
#include "lmetaevent.h"

struct TValue* getmt( struct TValue* t ){
	return NULL;
}

struct TValue* gettm( struct TValue* t, const TMS e ){
	struct TValue* mt = getmt( t );
	return NULL;
}

bool mt_call_bintm( struct TValue* tm, struct TValue* s, struct TValue* t
						, struct TValue* d ){
	return false;
} 

bool mt_call_binmevent( struct TValue* mt, struct TValue* s, struct TValue* t
					, struct TValue* d, int event ){
	return false;
}

bool call_binmevent( struct TValue* s, struct TValue* t, struct TValue* d
							, int event ){
	struct TValue* mt = getmt( s );
	if( !mt )
		getmt( t );

	if( !mt )
		return false;
	
	return mt_call_binmevent( mt, s, t, d, event );
}
