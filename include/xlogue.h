/*
* (C) Iain Fraser - GPLv3 
*
* Lua function prologue, epilogue and function call code generation. 
*/

#ifndef _XLOGUE_H_
#define _XLOGUE_H_

void epilogue( struct machine_ops* mop, struct emitter* e, struct frame* f );
void prologue( struct machine_ops* mop, struct emitter* e, struct frame* f );
void do_call( struct machine_ops* mop, struct emitter* e, struct frame* f, int vregbase, int narg, int nret );
void do_ret( struct machine_ops* mop, struct emitter* e, struct frame* f, int vregbase, int nret );

#endif
