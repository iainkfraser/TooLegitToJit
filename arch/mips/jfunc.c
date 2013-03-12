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
		case MJF_SAVENCALL:
			mips_jf_savencall( jf, me, m );
			break;
		default:
			assert( false );
	}
}

/*
* Spill/Fill MIPS temporary registers. 
*/

static bool is_mips_temp( operand o ){
	return ISO_REG( o ) && MIPSREG_ISTEMP( o.reg );
}

static void do_spill( struct emitter* me, struct frame* f, operand a, operand b, bool doswap ){
	if( doswap )
		swap( a, b );

	_MOP->move( me, f->m, a, b );
}

static void do_spill_or_fill( struct emitter* me, struct machine *m, bool isspill ){
	vreg_operand on,off;

	// phoney frame 
	struct frame F = { .m = m, .nr_locals = max_live_locals( m ), .nr_params = 0 };
	struct frame* f = &F;	
	
	for( int i = f->nr_locals; i >= 0; i-- ){
		on = vreg_to_operand( f, i, false );
		off = vreg_to_operand( f, i, true );

		if( is_mips_temp( on.value ) )
			do_spill( me, f, off.value, on.value, !isspill );

		if( is_mips_temp( on.type ) )
			do_spill( me, f, off.type, on.type, !isspill );
	}
}

void mips_jf_savencall( struct JFunc* jf, struct emitter* me, struct machine* m ){
	do_spill_or_fill( me, m, true );
	// TODO: do function call on v0
}
