/*
* (C) Iain Fraser - GPLv3  
* 32bit generic emitter. 
*/

#ifndef _EMITTER32_H_
#define _EMITTER32_H_

#include "emitter.h"

void emitter32_create( struct emitter** e, size_t vmlines );
void ENCODE_OP( struct emitter* e, uint32_t value );

#define ENCODE_DATA( mc, value )	ENCODE_OP( mc, value )
#define EMIT( x )			ENCODE_OP( me, x )


//#define REEMIT( x, y )	REENCODE_OP( me, x, y )

#endif

