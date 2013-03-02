/*
* (C) Iain Fraser - GPLv3  
* JIT Functions are functions used exclusively by JIT'r. 
*/

#include <stdbool.h>
#include <stdlib.h>
#include "jitfunc.h"
#include "macros.h"

static struct JFunc jit_functions[ JF_COUNT ];

struct emitter* jfuncs_create( struct machine* m, struct machine_ops* mop ){
	struct emitter* e;
	mop->create_emitter( &e, 0 );

	return e;
}

struct JFunc* jfuncs_get( int idx ){
	return &jit_functions[ idx ];
}

void jfuncs_cleanup(){
	struct JFunc *j = jit_functions;

	for( int i = 0; i < JF_COUNT; i++, j++ ){
		safe_free( j->params );
		safe_free( j->cregs );
	}

}

/*
* Thoughts:
*	Jumptable for func generators
*	All in one huge chunk of memory
*	No pc labels 
*/
