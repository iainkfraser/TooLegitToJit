/*
* (C) Iain Fraser - GPLv3 
*
* Platfrom indepdent vm and physical machine operand mapping.
*/

#ifndef _OPERAND_H_
#define _OPERAND_H_

#include <stdint.h>

/*
* Lua operand
*/

typedef struct lvm_operand{	// lua VM operand
	bool	islocal;
	int	index;	
} loperand;

loperand to_loperand( int rk );


/*
* Generic machine operand
*/

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

#define OP_TARGETREG( r )		{ .tag = OT_REG, { .reg = ( r ) } }
#define OP_TARGETDADDR( r, off ) 	{ .tag = OT_DIRECTADDR, { { .base = ( r ), .offset = ( off ) } } }
#define OP_TARGETIMMED( v )		{ .tag = OT_IMMED, { .k = ( v ) } }

/*
* Generic machine label
*/

typedef struct label {
	bool islocal;
	union {
		uint32_t	vline;		// pc label line 
		struct {			// local label id and direction (next/prev)
			uint8_t		local; 
			uint8_t		isnext;
		};
	};

} label;


#if 0
#define LBL_PC( l )			{ .islocal = false, { .vline = (l) } }
#define LBL_NEXT( l )			{ .islocal = true,  { { .local = (l), .isnext = true } } }
#define LBL_PREV( l )			{ .islocal = true,  { { .local = (l), .isnext = false } } }
#else
static inline label LBL_PC( uint32_t line ){ label l = { .islocal = false, { .vline = line } }; return l; }
static inline label LBL_NEXT( uint8_t local ){ label l = { .islocal = true }; l.local = local; l.isnext = true; return l; }
static inline label LBL_PREV( uint8_t local ){ label l = { .islocal = true }; l.local = local; l.isnext = false; return l; }
#endif


/*
* Generic machine Lua variable 
*/

typedef union vreg_operand {
	operand o[2];
	struct {
		operand value;
		operand type;
	};
} vreg_operand;

/* lua virtual reg and const mapping */
struct frame;

vreg_operand llocal_to_stack_operand( struct frame* f, int vreg );
vreg_operand llocal_to_operand( struct frame* f, int vreg );
vreg_operand lconst_to_operand( struct frame* f, int k );
vreg_operand loperand_to_operand( struct frame* f, loperand op );


#endif
