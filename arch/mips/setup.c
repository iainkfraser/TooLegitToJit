/*
* (C) Iain Fraser - GPLv3 
* Setup mips machine emitter. One per Lua function.
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "mc_emitter.h"
#include "arch/mips/regdef.h"
#include "arch/mips/opcodes.h"
#include "arch/mips/emitter.h"
#include "arch/mips/regmap.h"
#include "arch/mips/xlogue.h"
#include "arch/mips/vconsts.h"
#include "lopcodes.h"

void mce_init( void** mce, size_t vmlines ){
	const int jtsz = vmlines * 4;

	mips_emitter** me = ( mips_emitter**)mce;
	*me = malloc( sizeof( mips_emitter ) );
	(*me)->mcode = NULL;
	(*me)->size = 0;
	(*me)->bufsize = 0;
	(*me)->jt = malloc( jtsz );
	(*me)->cregs = 0;
	
	memset( (*me)->jt, 0, jtsz );
	INIT_LIST_HEAD( &(*me)->head );
}


void mce_proto_init( void** mce, size_t nr_protos ){
	mips_emitter** me = ( mips_emitter**)mce;
}


/* begin prologue and init data section */
void mce_start( void** mce , int nr_locals, int nr_params ){
	mips_emitter* me = *( mips_emitter**)mce;

	me->nr_locals = nr_locals;

	// write datasection first
//	const_write_section( me, nr_locals );
	
	// write epilogue - remember so we can reuse it 
	emit_epilogue( me, nr_locals );

	// finaly the prologue 
	emit_prologue( me, nr_locals, nr_params );
}



size_t mce_link( void** mce ){
	mips_emitter* me = *( mips_emitter**)mce;
	struct list_head *seek,*next;
	
	list_for_each_safe( seek , next, &me->head ){
		branch* b = list_entry( seek, branch, link );	
			
		// subtract 1 because of the delay slot
		int16_t offset = (int16_t)( me->jt[ b->vline ] - b->mline ) - 1;  
	
		// overwrite immediate operand	
		me->mcode[ b->mline ] = ( me->mcode[ b->mline ] & ~0xffff ) | ( offset & 0xffff );

		// remove ll node 
		list_del( seek ); 
		free( b );
	}
	return me->size;
}

void* mce_stop( void** mce, void* buf ){
	mips_emitter* me = *( mips_emitter**)mce;
	memcpy( buf, me->mcode, me->size );
	int jmp = me->pro * 4;

	free( me->mcode );
	free( me->jt );
	free( me );

	return (char*)buf + jmp; 
}

// call before emitting the branch
void push_branch( mips_emitter* me, int line ){
	branch *b = malloc( sizeof( branch ) );
	assert( b );
	b->mline = me->size / 4;
	b->vline = line;	
	list_add_tail( &b->link, &me->head );
}



void label( unsigned int pc , void** mce  ){
	mips_emitter* me = *( mips_emitter**)mce;
	me->jt[ pc ] = me->size / 4; 	
}


void mce_proto_set( void** mce, int pindex, void* addr ){
	
}
