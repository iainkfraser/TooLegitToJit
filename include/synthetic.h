/*
* (C) Iain Fraser - GPLv3  
* Sythentic generic machine instructions.
*/

#ifndef _SYNTHETIC_H_
#define _SYNTHETIC_H_

#include "machine.h"
#include "emitter.h"

void assign( struct machine_ops* mop, struct emitter* e, struct machine* m
					 	,vreg_operand d
						, vreg_operand s );

void loadim( struct machine_ops* mop, struct emitter* e, struct machine* m
						, int reg
						, int value ); 
void pushn( struct machine_ops* mop, struct emitter* e, struct machine* m
						, int nr_operands
						, ... );
void popn( struct machine_ops* mop, struct emitter* e, struct machine* m
						, int nr_operands
						, ... );
void address_of( struct machine_ops *mop, struct emitter *e, struct machine *m,
							operand d,
							operand s );
void vreg_fill( struct machine_ops *mop, struct emitter *e, struct frame *f,
							int vreg );
void vreg_spill( struct machine_ops *mop, struct emitter *e, struct frame *f,
							int vreg );
void vreg_type_fill( struct machine_ops *mop, struct emitter *e
					, struct frame * f
					, int vreg );

/*
* Predates JFunctions
*/
void syn_memcpyw( struct machine_ops* mop, struct emitter* e, struct machine* m, operand d, operand s, operand size );
void syn_memsetw( struct machine_ops* mop, struct emitter* e, struct machine* m, operand d, operand v, operand size );
void syn_min( struct machine_ops* mop, struct emitter* e, struct machine *m, operand d, operand s, operand t );
void syn_max( struct machine_ops* mop, struct emitter* e, struct machine *m, operand d, operand s, operand t );

#define push( mop, e, m, s )	pushn( mop, e, m, 1, s )
#define pop( mop, e, m, s )	popn( mop, e, m, 1, s )
#endif
