/*
* (C) Iain Fraser - GPLv3  
* JIT Functions are functions used exclusively by JIT'r. Currently
* implemented as a singleton probably will change in future. However
* requires a jfunc object passed to all jit functions.  
*/

#include <stdbool.h>
#include <stdlib.h>
#include "jitfunc.h"
#include "macros.h"

extern void jinit_cpy_arg_res( struct JFunc* jf, struct machine_ops* mop, struct emitter* e, struct machine* m );
extern void jinit_store_regs( struct JFunc* jf, struct machine_ops* mop, struct emitter* e, struct machine* m );
extern void jinit_load_regs( struct JFunc* jf, struct machine_ops* mop, struct emitter* e, struct machine* m );
extern void jinit_epi( struct JFunc* jf, struct machine_ops* mop, struct emitter* e, struct machine* m );
extern void jinit_pro( struct JFunc* jf, struct machine_ops* mop, struct emitter* e, struct machine* m );
extern void jinit_vresult_postcall( struct JFunc* jf, struct machine_ops* mop, struct emitter* e, struct machine* m );

#if 0
static struct JFunc jit_functions[ JF_COUNT ];
#else
static struct JFunc* jit_functions;
#endif

static char* jfuncs_code;
static jf_init jinittable[ JF_COUNT ] = {
	[ JF_ARG_RES_CPY ] = &jinit_cpy_arg_res
	,[ JF_STORE_LOCALS ] = &jinit_store_regs
	,[ JF_LOAD_LOCALS ] = &jinit_load_regs
	,[ JF_EPILOGUE ] = &jinit_epi
	,[ JF_PROLOGUE ] = &jinit_pro
	,[ JF_VARRES_POSTCALL ] = &jinit_vresult_postcall 
}; 



struct emitter* jfuncs_init( struct machine_ops* mop, struct machine* m, e_realloc era ){
	assert( !jit_functions );
	struct emitter *e;
	struct JFunc *j;
	const int n = mop->nr_jfuncs();
	
	jit_functions = malloc( sizeof( struct JFunc ) * ( JF_COUNT + n ) );
	assert( jit_functions );	// TODO: correct error handling 

	j = jit_functions;
	mop->create_emitter( &e, 0, era );

	for( int i = 0; i < JF_COUNT + n; i++, j++ ){
		j->addr = e->ops->ec( e );
	
		assert( temps_accessed( m ) == 0 );
		m->max_access =  0;

		if( i < JF_COUNT )
			( jinittable[i] )( j, mop, e, m );
		else
			mop->jf_init( j, e, m, i - JF_COUNT );

		j->temp_clobber = m->max_access;
	}

	return e;
}

void jfuncs_cleanup(){
	free( jit_functions );
}

void jfuncs_setsection( void* section ){
	jfuncs_code = (char*)section;
}

struct JFunc* jfuncs_get( int idx ){
	return &jit_functions[ idx ];
}

void* do_jfunc_addr( struct emitter* e, int idx, int off ){
	return e->ops->offset( e, jfuncs_get( idx )->addr + off, jfuncs_code );
}

void* jfunc_addr( struct emitter* e, int idx ){
	return do_jfunc_addr( e, idx, 0 );	
}

int jfuncs_temp_clobber( struct machine* m, int idx ){
	return min( jfuncs_get( idx )->temp_clobber, m->nr_temp_regs );
}

int jfuncs_stack_clobber( struct machine* m, int idx ){
	return max( jfuncs_get( idx )->temp_clobber - m->nr_temp_regs, 0 );		// +1 for the call saving ra
}

/*
* Call Jfunc[idx] with following constraints:
*	- Make sure call and JFunc total stack clobber is less than maxstack
*	- Make sure none of the input args are clobbered by the call.
* For faster compile times these constraints can't be skipped.  
*/
void jfunc_call( struct machine_ops* mop, struct emitter* e, struct machine* m, int idx, int off, int maxstack, int nargs, ... ){
	int callspill = 0;

	// TODO: check constraints 

	// max_access check spill
	mop->call( e, m, LBL_ABS( OP_TARGETIMMED( (uintptr_t)do_jfunc_addr( e, idx, off ) ) ) );	

	// check spill

	// TODO: proper error handling 
	assert( callspill + jfuncs_stack_clobber( m, idx )   < maxstack ); 
}


