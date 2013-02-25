/*
* (C) Iain Fraser - GPLv3 
*
* Platform independent stack manipulation interface.
*/

#ifndef _STACK_H_
#define _STACK_H_

#include "emitter.h"
#include "machine.h"

void store_frame( struct machine_ops* mop, struct emitter* e, struct frame* f );
void load_frame( struct machine_ops* mop, struct emitter* e, struct frame* f );

#endif 
