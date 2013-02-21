/*
* (C) Iain Fraser - GPLv3 
*
* MIPS mapping of 
*	constants <=> registers or immediate
*/

#include <stdlib.h>
#include <stdio.h>
#include "mc_emitter.h"
#include "arch/mips/regdef.h"
#include "arch/mips/emitter.h"
#include "arch/mips/mapping.h"

operand const_to_operand( struct mips_emitter* me, int k ){
	operand r;

	constant* c = &me->consts[ k ];
	if( c->isimmed ){
		r.tag = OT_IMMED;
		r.k = c->value;	
	} else if ( c->isspilled ) {
		r.tag = DATA_REG;
		r.base = _AT;
		r.offset = c->offset;
	} else {
		r.tag = OT_REG;
		r.reg = c->reg;
	}

	return r;
}

/*
* Process non-immediate constants and store in data section if needbe.
*/ 
void const_write_section( struct mips_emitter* me, int nr_locals ){
	struct constant* c;

	// spare physical registers 
	int reg = local_vreg_count( nr_locals );
	int spare_preg = NR_REGISTERS - reg; 


	for( int i = 0; i < me->constsize; i++ ){
		c = &me->consts[i]; 		

		if( c->isimmed )
			continue;

		switch( c->type ){
			/*
			* Try store in register otherwise make it immediate. It maybe tempting
			* to store in datasection BUT accessing cache ( or worse memory ) is much
			* slower then two instructions.
			*/
			case TNUMBER:	 
				if( spare_preg-- <= 0 )	break;
				c->isspilled = false;	
				c->reg = reg++;
				break;
		}	
		 		
	}
}

/*
* Load constants into registers.
*/
void const_emit_loadreg( struct mips_emitter* me ){
	struct constant* c;

	for( int i = 0; i < me->constsize; i++ ){
		c = &me->consts[i]; 		

		if( c->isimmed || c->isspilled ) 
			continue; 

		loadim( me, c->reg, c->value );
	}
}


void mce_const_init( void** mce, size_t nr_constants ){
	mips_emitter** me = ( mips_emitter**)mce;
	(*me)->consts = malloc( sizeof( struct constant ) * nr_constants );
	(*me)->constsize = nr_constants;
}

void mce_const_int( void** mce, int kindex, int value ){
	mips_emitter* me = *( mips_emitter**)mce;
	
	struct constant* c =  &me->consts[ kindex ];

	c->type = TNUMBER;
	c->isimmed = -32768 <= value && value <= 65535;
	c->value = value;
	
	if( !c->isimmed )
		me->cregs++;
}



