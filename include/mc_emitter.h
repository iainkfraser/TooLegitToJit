/*
* (C) Iain Fraser - GPLv3  
* Generic machine code emitter interface. One is used per
* function.
*/

#ifndef _MC_EMITTER_H_
#define _MC_EMITTER_H_

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "lopcodes.h"

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
	* TODO: Bitfield info 
	*/
	bool bitfield;
	

} operand;		// hmm good idea, could be platform opaque


typedef union vreg_operand {
	operand o[2];
	struct {
		operand value;
		operand type;
	};
} vreg_operand;


struct arch_emitter;

/* machine code emitter control interface */
void mce_init( struct arch_emitter** mce, size_t vmlines );
void mce_const_init( struct arch_emitter** mce, size_t nr_constants );
void mce_proto_init( struct arch_emitter** mce, size_t nr_protos );
void mce_proto_set( struct arch_emitter** mce, int pindex, void* addr );


#if 0
void mce_start( struct arch_emitter** mce, size_t vmlines );
#else
void mce_start( struct arch_emitter** mce, int nr_locals, int nr_params );
#endif

size_t mce_link( struct arch_emitter** mce );
void* mce_stop( struct arch_emitter** mce, void* buf );
void label( unsigned int pc , struct arch_emitter** Dst );


/* constant loading */
void mce_const_int( struct arch_emitter** mce, int kindex, int value );


/*
* Arch dependent 
*/


int arch_nr_locals( struct arch_emitter* mce );
vreg_operand arch_vreg_to_operand( int nr_locals, int vreg, bool stackonly );
vreg_operand arch_const_to_operand( struct arch_emitter* me, int k );

// TODO: this is emit LUA_MOVE
void assign( struct arch_emitter* me, vreg_operand d, vreg_operand s );


#include "machine.h"

#endif
