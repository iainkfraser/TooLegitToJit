/*
* (C) Iain Fraser - GPLv3 
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "regdef.h"
#include "mips_opcodes.h"
#include "mc_emitter.h"
#include "mips_emitter.h"
#include "mips_mapping.h"
#include "lopcodes.h"



void mce_init( void** mce, size_t vmlines, size_t nr_constants ){
	const int jtsz = vmlines * 4;

	mips_emitter** me = ( mips_emitter**)mce;
	*me = malloc( sizeof( mips_emitter ) );
	(*me)->mcode = NULL;
	(*me)->size = 0;
	(*me)->bufsize = 0;
	(*me)->jt = malloc( jtsz );
	(*me)->consts = malloc( sizeof( struct constant ) * nr_constants );
	(*me)->cregs = 0;
	(*me)->constsize = nr_constants;
	
	memset( (*me)->jt, 0, jtsz );
	INIT_LIST_HEAD( &(*me)->head );
}


void mce_const_int( void** mce, int kindex, int value ){
	mips_emitter* me = *( mips_emitter**)mce;
	
	struct constant* c =  &me->consts[ kindex ];

	c->type = TNUMBER;
	c->isimmed = -32768 <= value && value <= 65535;
	c->value = value;
	
	if( !c->isimmed )
		me->cregs++;
	
}

static inline constant* const_iter_end( mips_emitter* me ){
	return me->consts + me->constsize;
}

static struct constant* constant_iter_next( mips_emitter* me, struct constant* it, bool isimmed ){
	for( it++; it != const_iter_end( me ) && it->isimmed != isimmed; it++ )
		;

	return it;
}

static struct constant* constant_iter_forward( mips_emitter* me, struct constant* it, bool isimmed, int seek ){
	for( int i = 0; i < seek; i++ )
		it = constant_iter_next( me, it, isimmed );	
	return it;
}

static inline constant* constant_iter_begin( mips_emitter* me, bool isimmed ){
	return constant_iter_next( me, me->consts - 1, isimmed ); 
}

static inline constant* immed_iter_begin( mips_emitter* me ){ return constant_iter_begin( me, true ); }
static inline constant* immed_iter_next( mips_emitter* me, constant* it ){ return constant_iter_next( me, it, true ); }
static inline constant* immed_iter_forward( mips_emitter* me, constant* it, int seek ){ return constant_iter_forward( me, it, true, seek ); }
static inline constant* directaddr_iter_begin( mips_emitter* me ){ return constant_iter_begin( me, false ); }
static inline constant* directaddr_iter_next( mips_emitter* me, constant* it ){ return constant_iter_next( me, it, false ); }
static inline constant* directaddr_iter_forward( mips_emitter* me, constant* it, int seek ){ return constant_iter_forward( me, it, false, seek ); }


#define immed_iter_end		const_iter_end
#define directaddr_iter_end	const_iter_end

#define NR_TEMPREGS	10
#define NR_SAVEDREGS	8

static void xsreg( mips_emitter* me, int op, int savedreg, int stack_spill ){
	for( int i = 0; i < savedreg; i++ )
//		ENCODE_OP( me, GEN_MIPS_OPCODE_2REG( op, _sp, _s0 + i, ( stack_spill * 4 ) + ( i * 4 ) ) );
		ENCODE_OP( me, GEN_MIPS_OPCODE_2REG( op, _sp, _s0 + i, ( i * 4 ) ) );

}

static inline void store_sreg( mips_emitter* me, int savedreg, int stack_spill ){
	xsreg( me, MOP_SW, savedreg, stack_spill );
}

static inline void load_sreg( mips_emitter* me, int savedreg, int stack_spill ){
	xsreg( me, MOP_LW, savedreg, stack_spill );
}

static void epilogue( mips_emitter* me, int savedreg, int stack_spill ){
	printf("EPI\n");
	// save position for returns can jump here
	me->epi = me->size;  
	printf("%d\n",stack_spill);
	// load saved regs
	load_sreg( me, savedreg, stack_spill );

	printf("EPI\n");
		// remove frame
	// |move fp, sp
	// |lw fp,(0)sp
	ENCODE_OP( me, GEN_MIPS_OPCODE_3REG( MOP_SPECIAL, _fp, _zero, _sp, MOP_SPECIAL_OR ) );
	ENCODE_OP( me, GEN_MIPS_OPCODE_2REG( MOP_LW, _sp, _fp, (int16_t)-4 ) );
	ENCODE_OP( me, GEN_MIPS_OPCODE_2REG( MOP_LW, _sp, _ra, (int16_t)-8 ) );
	
	printf("EPI\n");
	// TODO: could move frame pointer load into delay slot 
	ENCODE_OP( me, GEN_MIPS_OPCODE_3REG( MOP_SPECIAL, _ra, 0, 0, MOP_SPECIAL_JR ) );
	ENCODE_OP( me, MOP_NOP );
	printf("EPI\n");
} 

static void prologue( mips_emitter* me, int nr_locals ){
	int stack_spill, data_nonspill, frame, savedreg, spilloff = 0;

	// store spilled nonimmediate constants to data section ( before code ) 
	data_nonspill = me->cregs - const_spill( nr_locals, me->cregs );
	constant* it = directaddr_iter_forward( me, directaddr_iter_begin( me ), data_nonspill );
	for( ; it != directaddr_iter_end( me ); it = directaddr_iter_next( me, it ), spilloff++ ){
		ENCODE_DATA( me, it->value );	

		it->isspilled = true;
		it->offset = spilloff;
	}


	// calculate frame mapping info
	savedreg = bound( 0, total_vreg_count( nr_locals, me->cregs ) - NR_TEMPREGS, NR_SAVEDREGS );
	stack_spill = local_spill( nr_locals );
	frame = 8 + stack_spill + savedreg; 		// 4 is for ebp and ra  
	
	// emit epilogue code
	epilogue( me, savedreg, stack_spill );
	
	// function actually starts here
	me->pro = me->size;
	
	// setup frame - function start position	
	//| sw fp, (0)sp
	//| move fp, sp
	//| addiu sp, sp, -frame  	// 16 bit unsigned 
	ENCODE_OP( me, GEN_MIPS_OPCODE_2REG( MOP_SW, _sp, _fp, (int16_t)-4 ) );
	ENCODE_OP( me, GEN_MIPS_OPCODE_2REG( MOP_SW, _sp, _ra, (int16_t)-8 ) );
	ENCODE_OP( me, GEN_MIPS_OPCODE_3REG( MOP_SPECIAL, _sp, _zero, _fp, MOP_SPECIAL_OR ) );
	ENCODE_OP( me, GEN_MIPS_OPCODE_2REG( MOP_ADDIU, _sp, _sp, (int16_t)( -frame ) ) );
	
	// store saved regs
	store_sreg( me, savedreg, stack_spill );	
 
	// load non spilled constants into registers 
	// NOT DOING THIS!
	if( data_nonspill > 0 ) {
		constant* end = directaddr_iter_forward( me, directaddr_iter_begin( me ), data_nonspill );
		int dstreg = vreg_to_reg( local_vreg_count( nr_locals ) ); 
		for( constant* it = directaddr_iter_begin( me ); it != end; it = directaddr_iter_next( me, it ), dstreg++ ){
			assert( !it->isimmed );
	//		ENCODE_OP( me, GEN_MIPS_OPCODE_2REG( MOP_LUI, 0, dstreg, ( it->value >> 16 ) & 0xffff ) );
	//		ENCODE_OP( me, GEN_MIPS_OPCODE_2REG( MOP_ORI, dstreg, dstreg, it->value & 0xffff ) );
			load_bigim( me, dstreg, it->value );			
	
			it->isspilled = false;
			it->offset = dstreg;
		}
	}
 
}

/* begin prologue and init data section */
void mce_start( void** mce , int nr_locals ){
	mips_emitter* me = *( mips_emitter**)mce;

	me->tregs = nr_locals + me->cregs;	
	printf("PREPROLOGUE\n");
	prologue( me, nr_locals );
	printf("POST\n");
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
	int jmp = me->pro;

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

operand const_to_operand( struct mips_emitter* me, int k ){
	operand r;

	constant* c = &me->consts[ k ];
	if( c->isimmed ){
		r.tag = OT_IMMED;
		r.k = c->value;	
	} else if ( c->isspilled ) {
		r.tag = OT_DIRECTADDR;
		r.base = _AT;
		r.offset = c->offset;
	} else {
		r.tag = OT_REG;
		r.reg = c->reg;
	}

	return r;
}

operand local_to_operand( struct mips_emitter* me, int l ){
	operand r;

	int vreg = local_vreg( l );
	printf("l:%d v:%d\n", l, vreg );

	if( vreg < NR_REGISTERS ) {	
		r.tag = OT_REG;
		r.reg = vreg_to_reg( vreg );
	} else { 
		r.tag = OT_DIRECTADDR;
		r.base = _sp;
		r.offset = 8 + ( vreg - NR_REGISTERS );		// saved registers stored at bottom of stack hence +8 
	}

	return r;
}


operand luaoperand_to_operand( struct mips_emitter* me, loperand op ){
	return op.islocal ? local_to_operand( me, op.index )  : const_to_operand( me, op.index );
}


int nr_slots( struct mips_emitter* me ){
	const int locals = me->tregs - me->cregs;	// locals need tag
	return local_vreg_count( locals ) + me->cregs;	
}

