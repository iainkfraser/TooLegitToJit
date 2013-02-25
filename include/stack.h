/*
* (C) Iain Fraser - GPLv3 
*
* Platform independent stack manipulation interface.
*/

#ifndef _STACK_H_
#define _STACK_H_

#include "emitter.h"

void store_frame( struct emitter* me, struct frame* f );
void load_frame( struct emitter* me, struct frame* f );

#endif 
