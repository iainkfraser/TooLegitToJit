/*
* (C) Iain Fraser - GPLv3 
*/

#ifndef _MIPS_EMITTER_H_
#define _MIPS_EMITTER_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "list.h"
#include "mc_emitter.h"

enum tag { TNUMBER = 0, TGARBAGE = 1, TNIL = 2, TUSER = 3, TNAT = 4 };	// NAT = not a type
#define NR_TAG_BITS	2	// number of bits needed to encode tag


typedef struct branch {
	uint32_t*		mcode;		// machine code instruction
	uint32_t		mline;		// machine code line
	uint32_t		vline;		// index into jump table i.e. vm line
	struct list_head	link;
} branch;

typedef struct constant {
	bool			isimmed;
	int			type;

	struct { 
		int 			value;
		struct {
			bool		isspilled;	// in data section
			union {
				int16_t		offset;		
				int16_t		reg;
			};
		};
	};
} constant;

typedef struct operand {
	enum { OT_REG, OT_DIRECTADDR, OT_IMMED } tag;	
	struct {
		struct {
			int16_t		offset;
			int 		base;
		};
		int		reg;
		int		k;	// immediate constant
	};

	/*
	* Bitfield info 
	*/
	bool bitfield;
	

} operand;

typedef struct mips_emitter {
	uint32_t*		mcode;		// machine code
	uint32_t*		jt;		// jump table
	size_t			nr_locals;	// number of virtual regs
	size_t 			size;		// size of machine code
	size_t			constsize;	// size of const array
	size_t			bufsize;	// size of machine code buffer
	struct constant*	consts;		// function constants
	int			cregs;		// number of constant virtual registers.
	uint32_t		epi;		// epilogue machine code pointer
	uint32_t		pro;		// prologue machine code pointer
	struct list_head	head;		// linked list of branch ops that need to be linked. 
} mips_emitter;

void push_branch( mips_emitter* me, int line );

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

#define GET_EC( mc )			( (mc)->size / 4 )		// emit counter 

#define DO_BRANCH( tec, sec )		( ( tec ) - ( sec + 1 ) )
#define BRANCH_FROM( mc, sec )		DO_BRANCH( GET_EC( mc ), sec )		
#define BRANCH_TO( mc, tec )		DO_BRANCH( tec, GET_EC( mc ) )

#define REENCODE_OP( mc, value, ec )						\
		do{								\
			(mc)->mcode[ (ec)  ] = (value);				\
		} while( 0 )

#define ENCODE_OP( mc, value )								\
	do{										\
		DASM_M_GROW( uint32_t, (mc)->mcode, (mc)->bufsize, (mc)->size + 4 );	\
		(mc)->mcode[ (mc)->size / 4 ] = value;					\
		(mc)->size += 4; 							\
	} while( 0 )

#define ENCODE_DATA( mc, value )	ENCODE_OP( mc, value )

#define EMIT( x )	ENCODE_OP( me, x )
#define REEMIT( x, y )	REENCODE_OP( me, x, y )

#define OP_TARGETREG( r )		{ .tag = OT_REG, { .reg = ( r ) } }
#define OP_TARGETDADDR( r, off ) 	{ .tag = OT_DIRECTADDR, { .base = ( r ), .offset = ( off ) } }

/*
* Util functions
*/

operand luaoperand_value_to_operand( struct mips_emitter* me, loperand op );
operand luaoperand_type_to_operand( struct mips_emitter* me, loperand op );
operand lualocal_value_to_operand( struct mips_emitter* me, int vreg );
operand lualocal_type_to_operand( struct mips_emitter* me, int vreg );
operand luak_value_to_operand( struct mips_emitter* me, int k );
operand luak_type_to_operand( struct mips_emitter* me, int k );


void load_bigim( struct mips_emitter* me, int reg, int k );
void loadim( struct mips_emitter* me, int reg, int k );
void do_assign( struct mips_emitter* me, operand d, operand s );

#endif
