/*
* (C) Iain Fraser - GPLv3  
* 32bit generic emitter. 
*/

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "list.h"
#include "emitter32.h"


typedef struct local_label {
	uint32_t		pc;
	struct list_head	link;
} local_label;

typedef struct branch {
	uint32_t*		mcode;		// machine code instruction
	uint32_t		mline;		// machine code line
	bool			islocal;
	union {
		uint32_t	vline;		// index into jump table i.e. vm line
		struct {			// jump to local
			uint8_t		local; 
			uint8_t		isnext;
		};
	};
	struct list_head	link;
} branch;


struct emitter32 {
	struct emitter	e;	
	uint32_t*		mcode;		// machine code
	uint32_t*		jt;		// jump table
	size_t 			size;		// size of machine code
	size_t			bufsize;	// size of machine code buffer
	struct list_head	head;		// linked list of branch ops that need to be linked. 
	struct list_head	local[NR_LOCAL_LABELS];	// local labels 
};

#define SELF	( (struct emitter32*) e )

static uint32_t find_local_label( struct emitter32* e, int pc, int local, bool isnext );

static size_t link( struct emitter* e ){
	struct list_head *seek,*next;
	uint32_t value;
	int16_t offset;

	list_for_each_safe( seek , next, &SELF->head ){
		branch* b = list_entry( seek, branch, link );	
		
		if( b->islocal )
			offset = find_local_label( SELF, b->mline, b->local, b->isnext );
		else	
			offset = (int16_t)( SELF->jt[ b->vline ] - b->mline ) - 1;  
	
		// calculate new value
		value = ( SELF->mcode[ b->mline ] & ~0xffff ) | ( offset & 0xffff );
	
		// overwrite immediate operand	
		SELF->mcode[ b->mline ] = value;

		// remove ll node 
		list_del( seek ); 
		free( b );
	}

	return SELF->size;
}

static void* stop( struct emitter* e, void* buf, unsigned int startec ){
	memcpy( buf, SELF->mcode, SELF->size );
	int jmp = startec * 4;

	// TODO: free the local labels list

	free( SELF->mcode );
	free( SELF->jt );
	free( SELF );

	return (char*)buf + jmp; 
}

static void label_pc( struct emitter* e, unsigned int pc ){
	SELF->jt[ pc ] = SELF->size / 4; 	
}

static void label_local( struct emitter* e, unsigned int local ){
	assert( 0 <= local && local < NR_LOCAL_LABELS );
	
	struct list_head* seek;
	const uint32_t pc = e->ops->ec( e );

	struct local_label* ll = malloc( sizeof( struct local_label ) );
	assert( ll );			// TODO: error handling 

	// insert in order
	list_for_each( seek , &SELF->local[ local ] ){
		local_label* sll = list_entry( seek, local_label, link );	
		if( pc < sll->pc )
			goto insert;
	
	}

insert:
	list_add_tail( &ll->link, &SELF->local[ local ]);
}


static void branch_pc( struct emitter* e, int line ){
	branch *b = malloc( sizeof( branch ) );
	assert( b );			// TODO: error handling 
	b->mline = e->ops->ec( e ); 	//SELF->size / 4;
	b->vline = line;	
	b->islocal = false;
	list_add_tail( &b->link, &SELF->head );
}

void branch_local( struct emitter* e, int local, bool isnext ){
	branch *b = malloc( sizeof( branch ) );
	assert( b );			// TODO: error handling 
	b->mline = e->ops->ec( e ); 	//SELF->size / 4;
	b->local = local;
	b->isnext = isnext;	
	b->islocal = true;
	list_add_tail( &b->link, &SELF->head );
}


static unsigned int ec( struct emitter* e ){
	return SELF->size / 4;
}

static struct emitter_ops vtable = {
	.link = &link,
	.stop = &stop,
	.label_pc = &label_pc,
	.label_local = &label_local,
	.branch_pc = &branch_pc,
	.branch_local = &branch_local, 
	.ec = &ec
};


void emitter32_create( struct emitter** e, size_t vmlines ){
	const int jtsz = vmlines * 4;

	*e = malloc( sizeof( struct emitter32 ) );
	assert( *e );			// TODO: error checking 	
	
	struct emitter32* self = ( struct emitter32* ) *e;

	self->e.ops = &vtable;
	self->mcode = NULL;
	self->size = 0;
	self->bufsize = 0;
	self->jt = malloc( jtsz );

	assert( self->jt );		// TODO: error checking

	memset( self->jt, 0, jtsz );

	INIT_LIST_HEAD( &self->head );
	for( int i = 0; i < NR_LOCAL_LABELS; i++ )
		INIT_LIST_HEAD( &self->local[i] ); 

}



static uint32_t find_local_label( struct emitter32* e, int pc, int local, bool isnext ) {
	struct list_head* seek;
	list_for_each( seek , &e->local[ local ] ){
		local_label* sll = list_entry( seek, local_label, link );	
		
		if( pc >= sll->pc && !isnext )
			return sll->pc;

		if( pc <= sll->pc && isnext  )
			return sll->pc;
	
	}

	assert( false );

}

/*
* Emitter specilisations 
*/


#define DASM_M_GROW( t, p, sz, need) \
  do { \
    size_t _sz = (sz), _need = (need); \
    if (_sz < _need) { \
      if (_sz < 16) _sz = 16; \
      while (_sz < _need) _sz += _sz; \
      (p) = (t *)realloc((p), _sz); \
      if ((p) == NULL) exit(1); \
      (sz) = _sz; \
    } \
  } while(0)


void ENCODE_OP( struct emitter* e, uint32_t value ) {
	DASM_M_GROW( uint32_t, SELF->mcode, SELF->bufsize, SELF->size + 4 );
	SELF->mcode[ SELF->size / 4 ] = value;
	SELF->size += 4; 	
} 



