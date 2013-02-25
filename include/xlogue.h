/*
* (C) Iain Fraser - GPLv3 
*
* Lua function prologue and epilogue code generation. 

* Functions are called after Lua constants are loaded ( see frame.c )
* and processed.  
*/

#ifndef _XLOGUE_H_
#define _XLOGUE_H_

void epilogue( struct machine_ops* mop, struct emitter* e, struct frame* f );
void prologue( struct machine_ops* mop, struct emitter* e, struct frame* f );

#endif
