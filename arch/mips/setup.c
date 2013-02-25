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

static uint32_t find_local_label( int pc, int local, struct arch_emitter* me, bool isnext );

void mce_init( arch_emitter** mce, size_t vmlines ){
	const int jtsz = vmlines * 4;

	arch_emitter** me = ( arch_emitter**)mce;
	*me = malloc( sizeof( arch_emitter ) );
	(*me)->mcode = NULL;
	(*me)->size = 0;
	(*me)->bufsize = 0;
	(*me)->jt = malloc( jtsz );
	(*me)->cregs = 0;
	
	memset( (*me)->jt, 0, jtsz );
	INIT_LIST_HEAD( &(*me)->head );
}

size_t mce_link( arch_emitter** mce ){
	arch_emitter* me = *( arch_emitter**)mce;
	struct list_head *seek,*next;
	uint32_t value;
	int16_t offset;

	list_for_each_safe( seek , next, &me->head ){
		branch* b = list_entry( seek, branch, link );	
		
		if( b->islocal )
			offset = find_local_label( b->mline, b->local, me, b->isnext );
		else	
			offset = (int16_t)( me->jt[ b->vline ] - b->mline ) - 1;  
	
		// calculate new value
		value = ( me->mcode[ b->mline ] & ~0xffff ) | ( offset & 0xffff );
	
		// overwrite immediate operand	
		me->mcode[ b->mline ] = value;

		// remove ll node 
		list_del( seek ); 
		free( b );
	}
	return me->size;
}

void* mce_stop( arch_emitter** mce, void* buf ){
	arch_emitter* me = *( arch_emitter**)mce;
	memcpy( buf, me->mcode, me->size );
	int jmp = me->pro * 4;

	free( me->mcode );
	free( me->jt );
	free( me );

	return (char*)buf + jmp; 
}

// call before emitting the branch
void push_branch( arch_emitter* me, int line ){
	branch *b = malloc( sizeof( branch ) );
	assert( b );			// TODO: error handling 
	b->mline = me->size / 4;
	b->vline = line;	
	b->islocal = false;
	list_add_tail( &b->link, &me->head );
}

void push_branch_local( arch_emitter* me, int local, bool isnext ){
	branch *b = malloc( sizeof( branch ) );
	assert( b );			// TODO: error handling 
	b->mline = me->size / 4;
	b->local = local;
	b->isnext = isnext;	
	b->islocal = true;
	list_add_tail( &b->link, &me->head );
}

void label( unsigned int pc , arch_emitter** mce  ){
	arch_emitter* me = *( arch_emitter**)mce;
	me->jt[ pc ] = me->size / 4; 	
}

static uint32_t find_local_label( int pc, int local, struct arch_emitter* me, bool isnext ){
	struct list_head* seek;
	list_for_each( seek , &me->local[ local ] ){
		local_label* sll = list_entry( seek, local_label, link );	
		
		if( pc >= sll->pc && !isnext )
			return sll->pc;

		if( pc <= sll->pc && isnext  )
			return sll->pc;
	
	}

	assert( false );

}

void label_local( int local, struct arch_emitter** mce ){
	assert( 0 <= local && local < NR_LOCAL_LABELS );
	arch_emitter* me = *( arch_emitter**)mce;
	
	struct list_head* seek;
	const uint32_t pc = mce_ec( mce );

	struct local_label* ll = malloc( sizeof( struct local_label ) );
	assert( ll );			// TODO: error handling 

	// insert in order
	list_for_each( seek , &me->local[ local ] ){
		local_label* sll = list_entry( seek, local_label, link );	
		if( pc < sll->pc )
			goto insert;
	
	}

insert:
	list_add_tail( &ll->link, &me->local[ local ]);
}
