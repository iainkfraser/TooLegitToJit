/*
* (C) Iain Fraser - GPLv3  
* MIPS jit functions.
*/

#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include "machine.h"
#include "macros.h"
#include "emitter32.h"
#include "bit_manip.h"
#include "frame.h"
#include "jitfunc.h"
#include "stack.h"
#include "arch/mips/machine.h"
#include "arch/mips/regdef.h"
#include "arch/mips/opcodes.h"
#include "arch/mips/arithmetic.h"
#include "arch/mips/call.h"
#include "arch/mips/branch.h"
#include "arch/mips/jfunc.h"

int mips_nr_jfuncs(){
	return MJF_COUNT;
}

void mips_jf_init( struct JFunc* jf, struct emitter* me, struct machine* m, int idx ){
	switch( idx ){
		case MJF_STORETEMP:
			mips_jf_storetemp( jf, me, m );
			break;
		case MJF_LOADTEMP:
			mips_jf_loadtemp( jf, me, m );
			break;
		default:
			assert( false );
	}
}

/*
* Spill/Fill MIPS temporary registers. 
*/


static void do_spill( struct emitter* me, struct frame* f, operand a, operand b, bool doswap ){
	if( doswap )
		swap( a, b );

	_MOP->move( me, f->m, a, b );
}

static void do_spill_or_fill( struct JFunc* jf, struct emitter* me, struct machine *m, bool isspill ){
	int start,end, spill = 0;
	vreg_operand on,off;

	// phoney frame 
	struct frame F = { .m = m, .nr_locals = max_live_locals( m ), .nr_params = 0 };
	struct frame* f = &F;	

	start =  me->ops->ec( me );
	
	for( int i = f->nr_locals; i >= 0; i-- ){
		on = vreg_to_operand( f, i, false );
		off = vreg_to_operand( f, i, true );

		if( is_mips_temp( on.value ) )
			do_spill( me, f, off.value, on.value, !isspill ), spill++;

		if( is_mips_temp( on.type ) )
			do_spill( me, f, off.type, on.type, !isspill ), spill++;
	}

	end = me->ops->ec( me ); 

	
	_MOP->ret( me, m );

	assert( ( end - start ) % spill == 0 );
	jf->data = ( end - start ) / spill;	
}

void mips_jf_storetemp( struct JFunc* jf, struct emitter* me, struct machine* m ){
	do_spill_or_fill( jf, me, m, true );
}

void mips_jf_loadtemp( struct JFunc* jf, struct emitter* me, struct machine* m ){
	do_spill_or_fill( jf, me, m, false );
}

int mjf_loadtemp_offset( struct machine* m, int nr_locals ){
	struct JFunc* j = jfuncs_get( jf_arch_idx( MJF_LOADTEMP ) );
	return max( 0, ( m->nr_reg - m->nr_temp_regs ) - 2 * nr_locals ) * j->data; 
}

int mjf_storetemp_offset( struct machine* m, int nr_locals ){
	struct JFunc* j = jfuncs_get( jf_arch_idx( MJF_STORETEMP ) );
	return max( 0, ( m->nr_reg - m->nr_temp_regs ) - 2 * nr_locals ) * j->data; 
}
