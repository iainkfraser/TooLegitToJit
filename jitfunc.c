/*
* (C) Iain Fraser - GPLv3  
* JIT Functions are functions used exclusively by JIT'r. 
*/

#include <stdbool.h>
#include <stdlib.h>
#include "jitfunc.h"
#include "macros.h"

extern void jinit_cpy_arg_res( struct JFunc* jf, struct machine_ops* mop, struct emitter* e, struct machine* m );

static struct JFunc jit_functions[ JF_COUNT ];
static char* jfuncs_code;
static jf_init jinittable[ JF_COUNT ] = {
	[ JF_ARG_RES_CPY ] = &jinit_cpy_arg_res 
}; 



struct emitter* jfuncs_init( struct machine_ops* mop, struct machine* m ){
	struct emitter* e;
	struct JFunc *j = jit_functions;

	mop->create_emitter( &e, 0 );

	for( int i = 0; i < JF_COUNT; i++ ){
		j->addr = e->ops->ec( e );
	
		assert( temps_accessed( m ) == 0 );
		m->max_access =  0;

		( jinittable[i] )( j, mop, e, m );

		j->temp_clobber = m->max_access;
	}

	return e;
}

void jfuncs_cleanup(){
	struct JFunc *j = jit_functions;
	;
}

void jfuncs_setsection( void* section ){
	jfuncs_code = (char*)section;
}

static inline struct JFunc* jfuncs_get( int idx ){
	return &jit_functions[ idx ];
}

void* jfuncs_addr( struct machine* m, int idx ){
	jfuncs_get( idx )->addr + jfuncs_get( idx )->addr; 
}

int jfuncs_temp_clobber( struct machine* m, int idx ){
	min( jfuncs_get( idx )->temp_clobber, m->nr_temp_regs );
}

int jfuncs_stack_clobber( struct machine* m, int idx ){
	return max( jfuncs_get( idx )->temp_clobber - m->nr_temp_regs, 0 );
}
