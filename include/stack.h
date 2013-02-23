/*
* (C) Iain Fraser - GPLv3 
*
* Platform independent stack manipulation interface.
*/

#ifndef _STACK_H_
#define _STACK_H_

#include "mc_emitter.h"

void store_frame( struct arch_emitter* me );
void load_frame( struct arch_emitter* me );

#endif 
