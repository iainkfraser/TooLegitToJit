/*
* (C) Iain Fraser - GPLv3 
*
* Platform independent function/frame manipulation code.
*/

#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include "frame.h"
#include "lopcodes.h"
#include "xlogue.h"
#include "lobject.h"

void init_consts( struct frame* f, int n ){
	f->consts = malloc( sizeof( operand ) * n );
	// TODO: error handling 
}

void setk_number( struct frame* f, int k, int value ) {
	f->consts[ k ].tag = OT_IMMED;
	f->consts[ k ].k = value;
}


void emit_header( struct machine_ops* mop, struct emitter* e, struct frame* f ){
	// write data section for strings

	// write epilogue and rem location
	f->epi = e->ops->ec( e );
	epilogue( mop, e, f );

	// write prologue and rem location
	f->pro = e->ops->ec( e );
	prologue( mop, e, f );

	// TODO: check for large immed that take 2 instructions and store some in register if available 
	// load constants
//	const_emit_loadreg( me );
}

void emit_footer( struct machine_ops* mop, struct emitter* e, struct frame* f ){
	free( f->consts );
}

/*
* Load 
*/


/*
* Local and constant mapping 
*/

#define INVERSE( i, n )	( n - ( i + 1 ) )
#define NR_SPARE_REGS( f )	( (f)->m->nr_reg - (f)->m->nr_temp_regs )

intptr_t vreg_value_offset( int idx ){
	return -( idx * 8 );
}

intptr_t vreg_type_offset( int idx ){
	return -( idx * 8 + 4 );
}

struct TValue* vreg_tvalue_offset( struct TValue* base, int idx ){
	return base - idx;
} 


static int vreg_to_physical_reg( struct frame* f, int vreg ){
	assert( f );
	assert( f->m );
	assert( vreg + f->m->nr_temp_regs < f->m->nr_reg );
	return f->m->reg[ f->m->nr_temp_regs + vreg ];
}


static operand pslot_to_operand( struct frame* f, int nr_locals, int pidx, bool stackonly ){
	assert( f );
	assert( f->m );

	if( stackonly || pidx >= NR_SPARE_REGS( f ) ){
#if 0
		operand r = OP_TARGETDADDR( f->m->sp, 4 * INVERSE( pidx,  2 * nr_locals ) );
#else
		operand r = OP_TARGETDADDR( f->m->fp, -(8 + 4 * pidx) );
#endif
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
	o.value = f->consts[ k ];
	o.type = OP_TARGETIMMED( LUA_TNUMBER );
	return o;
}

int code_start( struct frame* f ){
	return f->pro; 
}

