/*
* (C) Iain Fraser - GPLv3  
* MIPS branch instructions implementation. 
*/

#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include "machine.h"
#include "macros.h"
#include "emitter32.h"
#include "bit_manip.h"
#include "arch/mips/regdef.h"
#include "arch/mips/opcodes.h"
#include "arch/mips/arithmetic.h"

extern void move( struct emitter* me, struct machine* m, operand d, operand s );

struct machine_ops mips_ops;
#define _MOP	( &mips_ops )


static int branch( struct emitter* me, label l ){
	switch( l.tag ){
		case LABEL_LOCAL:
			me->ops->branch_local( me, l.local, l.isnext );
			break;
		case LABEL_PC:
			me->ops->branch_pc( me, l.vline );
			break;
		case LABEL_EC:
			return l.ec - ( me->ops->ec( me ) + 1 ); 		
	}

	return 0;
}

void mips_b( struct emitter* me, struct machine* m, label l ){
	EMIT( MI_B( branch( me, l ) ) );
	EMIT( MI_NOP() );
}


void mips_beq( struct emitter* me, struct machine* m, operand d, operand s, label l ){
	int temps = load_coregisters( _MOP, me, m, 2, &d, &s );

	EMIT( MI_BEQ( d.reg, s.reg, branch( me, l ) ) );
	EMIT( MI_NOP( ) );
	
	unload_coregisters( _MOP, me, m, temps );
}

void mips_blt( struct emitter* me, struct machine* m, operand d, operand s, label l ){
	operand otemp = OP_TARGETREG( acquire_temp( _MOP, me, m ) );
	assert( otemp.tag == OT_REG );

	mips_sub( me, m, otemp, d, s );
	
	EMIT( MI_BLTZ( otemp.reg, branch( me, l ) ) );
	EMIT( MI_NOP( ) );

	release_temp( _MOP, me, m );
}

void mips_bgt( struct emitter* me, struct machine* m, operand d, operand s, label l ){
	operand otemp = OP_TARGETREG( acquire_temp( _MOP, me, m ) );
	assert( otemp.tag == OT_REG );

	mips_sub( me, m, otemp, d, s );
	
	EMIT( MI_BGTZ( otemp.reg, branch( me, l ) ) );
	EMIT( MI_NOP( ) );

	release_temp( _MOP, me, m );
}

void mips_ble( struct emitter* me, struct machine* m, operand d, operand s, label l ){
	;
}

void mips_bge( struct emitter* me, struct machine* m, operand d, operand s, label l ){
	operand otemp = OP_TARGETREG( acquire_temp( _MOP, me, m ) );
	assert( otemp.tag == OT_REG );

	mips_sub( me, m, otemp, d, s );
	
	EMIT( MI_BGEZ( otemp.reg, branch( me, l ) ) );
	EMIT( MI_NOP( ) );

	release_temp( _MOP, me, m );
}



