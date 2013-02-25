/*
* (C) Iain Fraser - GPLv3 
*
* Lua function prologue and epilogue code generation. 

* Functions are called after Lua constants are loaded ( see frame.c )
* and processed.  
*/

#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include "frame.h"
#include "machine.h"
#include "lopcodes.h"

static int stack_frame_size( int nr_locals ){
	int k = 4 * 4;	// ra, closure, return position, nr results 
	return nr_locals * 8 + k;
}

void epilogue( struct machine_ops* mop, struct emitter* e, struct frame* f ){
	const int rsp = f->m->sp;
	operand osp = OP_TARGETREG( rsp );

	mop->add( e, f->m, osp, osp, OP_TARGETIMMED( stack_frame_size( f->nr_locals ) ) ); 
	mop->move( e, f->m, OP_TARGETREG( temp_reg( f->m, 0 ) ), OP_TARGETDADDR( rsp, -4 ) ); 
	mop->move( e, f->m, OP_TARGETREG( temp_reg( f->m, 1 ) ), OP_TARGETDADDR( rsp, -8 ) ); 
	mop->ret( e, f->m );
}


void prologue( struct machine_ops* mop, struct emitter* e, struct frame* f ){
	const int rsp = f->m->sp;
	operand osp = OP_TARGETREG( rsp );

	// load the paramaters 
//	emit_load_params( me, nr_locals, nr_params );

	mop->move( e, f->m, OP_TARGETDADDR( rsp, -4 ), OP_TARGETREG( temp_reg( f->m, 0 ) ) ); 
	mop->move( e, f->m, OP_TARGETDADDR( rsp, -8 ), OP_TARGETREG( temp_reg( f->m, 2 ) ) ); 
	mop->move( e, f->m, OP_TARGETDADDR( rsp, -12 ), OP_TARGETREG( temp_reg( f->m, 3 ) ) ); 
	mop->add( e, f->m, osp, osp, OP_TARGETIMMED( -stack_frame_size( f->nr_locals ) ) ); 

	// save the return address, closure, return position and expected number of args
#if 0
	EMIT( MI_SW( _ra, _sp, -4 ) );
	EMIT( MI_SW( _a0, _sp, -8 ) );		// closure
	EMIT( MI_SW( _a1, _sp, -12 ) );		// base result virtual register
	EMIT( MI_SW( _a3, _sp, -16 ) );		// number of results 

	// update stack
	EMIT( MI_ADDIU( _sp, _sp, -stack_frame_size( nr_locals ) ) );

#endif 
}

