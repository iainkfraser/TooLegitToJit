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
#include "arch/mips/regmap.h"
#include "arch/mips/vstack.h"

vreg_operand const_to_operand( struct arch_emitter* me, int k ){
	vreg_operand ret;
	operand r;

	ret.type.tag = OT_IMMED;
	ret.type.k = TNUMBER;	

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

	ret.value = r;
	return ret;
}

vreg_operand arch_const_to_operand( struct arch_emitter* me, int k ){
	return const_to_operand( me, k );
}

/*
* Process non-immediate constants and store in data section if needbe.
*/ 
void const_write_section( struct arch_emitter* me, int nr_locals ){
	struct constant* c;

	// spare physical registers 
	int reg = nr_livereg_vreg_occupy( nr_locals ); 
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
void const_emit_loadreg( struct arch_emitter* me ){
	struct constant* c;

	for( int i = 0; i < me->constsize; i++ ){
		c = &me->consts[i]; 		

		if( c->isimmed || c->isspilled ) 
			continue; 

		loadim( me, c->reg, c->value );
	}
}


void mce_const_init( arch_emitter** mce, size_t nr_constants ){
	arch_emitter** me = ( arch_emitter**)mce;
	(*me)->consts = malloc( sizeof( struct constant ) * nr_constants );
	(*me)->constsize = nr_constants;
}

void mce_const_int( arch_emitter** mce, int kindex, int value ){
	arch_emitter* me = *( arch_emitter**)mce;
	
	struct constant* c =  &me->consts[ kindex ];

	c->type = TNUMBER;
	c->isimmed = -32768 <= value && value <= 65535;
	c->value = value;
	
	if( !c->isimmed )
		me->cregs++;
}



