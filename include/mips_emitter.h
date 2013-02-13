/*
* (C) Iain Fraser - GPLv3 
*/

#ifndef _MIPS_EMITTER_H_
#define _MIPS_EMITTER_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "list.h"


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

	union { 
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
} operand;

typedef struct mips_emitter {
	uint32_t*		mcode;		// machine code
	uint32_t*		jt;		// jump table
	size_t 			size;		// size of machine code
	size_t			constsize;	// size of const array
	size_t			bufsize;	// size of machine code buffer
	struct constant*	consts;		// function constants
	int			cregs;		// number of constant virtual registers.
	int			tregs;		// total virtual regs i.e. locals + non immediate registers
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


#define ENCODE_OP( mc, value )								\
	do{										\
		DASM_M_GROW( uint32_t, (mc)->mcode, (mc)->bufsize, (mc)->size + 4 );	\
		(mc)->mcode[ (mc)->size / 4 ] = value;					\
		(mc)->size += 4; 							\
	} while( 0 )

#define ENCODE_DATA( mc, value )	ENCODE_OP( mc, value )

/*
* Util functions
*/
operand const_to_operand( struct mips_emitter* me, int k );
operand local_to_operand( struct mips_emitter* me, int l );
operand luaoperand_to_operand( struct mips_emitter* me, loperand op );
int nr_slots( struct mips_emitter* me );
void load_bigim( struct mips_emitter* me, int reg, int k );
void emit_gettable( void** mce, loperand dst, loperand table, loperand idx );
#endif
