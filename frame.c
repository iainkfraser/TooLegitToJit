/*
* (C) Iain Fraser - GPLv3 
*
* Platform independent function/frame manipulation code.
*/

#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include "frame.h"

void init_consts( struct frame* f, int n ){
	f->consts = malloc( sizeof( operand ) * n );
	// TODO: error handling 
}

void setk_number( struct frame* f, int k, int value ) {
	f->consts[ k ].tag = OT_IMMED;
	f->consts[ k ].k = value;
}


void emit_header( struct emitter* ae, struct frame* f ){
	// write data section for strings

	// write epilogue and rem location
//	f->epi = mce_ec( &ae );

	// write prologue and rem location
//	f->pro = mce_ec( &ae );

	// TODO: check for large immed that take 2 instructions and store some in register if available 
}

void emit_footer( struct emitter* ae, struct frame* f ){
	free( f->consts );
}

/*
* Load 
*/


/*
* Local and constant mapping 
*/

#define INVERSE( i, n )	( n - ( i + 1 ) )
#define NR_SPARE_REGS( f )	( (f)->m->nr_reg - (f)->nr_temp_regs )

static int vreg_to_physical_reg( struct frame* f, int vreg ){
	assert( f );
	assert( f->m );
	assert( vreg + f->nr_temp_regs < f->m->nr_reg );
	return f->m->reg[ f->nr_temp_regs + vreg ];
}


static operand pslot_to_operand( struct frame* f, int nr_locals, int pidx, bool stackonly ){
	assert( f );
	assert( f->m );

	if( stackonly || pidx >= NR_SPARE_REGS( f ) ){
		operand r = OP_TARGETDADDR( f->m->sp, 4 * INVERSE( pidx,  2 * nr_locals ) );
		return r;
	} else {
		operand r = OP_TARGETREG( vreg_to_physical_reg( f, pidx ) );
		return r;
	}
			
} 

vreg_operand vreg_to_operand( struct frame* f, int vreg, bool stackonly ){
	assert( f );

	const int pidx = vreg * 2;
	vreg_operand vo;
	
	vo.value = pslot_to_operand( f, f->nr_locals, pidx, stackonly );
	vo.type = pslot_to_operand( f, f->nr_locals, pidx + 1, stackonly );

	return vo;
}

vreg_operand const_to_operand( struct frame* f, int k ){
	vreg_operand o;
	return o;
}
