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


bool do_call_binmevent( struct TValue* mt, struct TValue* s, struct TValue* t,
					struct TValue* d, int event ){
	;
}

bool call_binmevent( struct TValue* s, struct TValue* t, struct TValue* d
							, int event ){
	struct TValue* mt = getmt( s );
	if( !mt )
		getmt( t );

	if( !mt )
		return false;
	
	return do_call_binmevent( mt, s, t, d, event );
}
