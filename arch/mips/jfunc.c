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
