/*
* (C) Iain Fraser - GPLv3 
*
* Mapping constants to registers and data section ( spilled ).
*/

#ifndef _VCONST_H_
#define _VCONST_H_

void const_write_section( struct arch_emitter* me, int nr_locals );
void const_emit_loadreg( struct arch_emitter* me );

#endif
