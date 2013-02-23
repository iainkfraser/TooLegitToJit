/*
* (C) Iain Fraser - GPLv3 
*
* Mapping virtual registers to live and offline positions ( spilled ).
*/

#ifndef _VSTACK_H_
#define _VSTACK_H_

// TODO: platform indepdenent interface?



int nr_livereg_vreg_occupy( int nr_locals );

operand vreg_compiletime_stack_value( int nr_locals, int vreg );
operand vreg_compiletime_stack_type( int nr_locals, int vreg );
operand vreg_compiletime_reg_value( int nr_locals, int vreg );
operand vreg_compiletime_reg_type( int nr_locals, int vreg );

void vreg_runtime_reg_value( struct arch_emitter* me, int nr_locals, int ridx, int rout );
void vreg_runtime_reg_type( int nr_locals, int vreg );
void vreg_runtime_stack_value( struct arch_emitter* me, int nr_locals, int ridx, int rout );
void vreg_runtime_stack_type( struct arch_emitter* me, int nr_locals, int ridx, int rout );


void vreg_runtime_stack_value_load( struct arch_emitter* me, int rstack, int roff, int ioff, int rout );
void vreg_runtime_stack_type_load( struct arch_emitter* me, int rstack, int roff, int ioff, int rout );
void vreg_runtime_stack_value_store( struct arch_emitter* me, int rstack, int roff, int ioff, int rvalue );
void vreg_runtime_stack_type_store( struct arch_emitter* me, int rstack, int roff, int ioff, int rvalue );

void store_frame( struct arch_emitter* me );
void load_frame( struct arch_emitter* me );

#endif

