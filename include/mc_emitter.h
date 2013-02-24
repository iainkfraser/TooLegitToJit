/*
* (C) Iain Fraser - GPLv3  
* Pure machine code emitter
*/

#ifndef _MC_EMITTER_H_
#define _MC_EMITTER_H_

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "lopcodes.h"


struct arch_emitter;

/* machine code emitter control interface */
void mce_init( struct arch_emitter** mce, size_t vmlines );
void mce_proto_init( struct arch_emitter** mce, size_t nr_protos );
size_t mce_link( struct arch_emitter** mce );
void* mce_stop( struct arch_emitter** mce, void* buf );
void label( unsigned int pc , struct arch_emitter** Dst );

size_t mce_ec( struct arch_emitter** mce );

#endif
