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

#if 0
#define OP_TARGETREG( r )		{ .tag = OT_REG, { .reg = ( r ) } }
#define OP_TARGETDADDR( r, off ) 	{ .tag = OT_DIRECTADDR, { { .base = ( r ), .offset = ( off ) } } }
#define OP_TARGETIMMED( v )		{ .tag = OT_IMMED, { .k = ( v ) } }
#else
static inline operand OP_TARGETREG( int r ){ 
	operand o = { .tag = OT_REG, { .reg = ( r ) } }; return o; 
}

static inline operand OP_TARGETDADDR( int b, int off ){
	operand o = { .tag = OT_DIRECTADDR, { { .base = ( b ), .offset = ( off ) } } };
	return o;
}

static inline operand OP_TARGETIMMED( int k ){
	operand o = { .tag = OT_IMMED, { .k = ( k ) } };
	return o;
}
#endif

/*
* Generic machine label
*/

typedef struct label {
	enum{ LABEL_PC, LABEL_LOCAL, LABEL_EC } tag;

	union {
		uint32_t	vline;		// pc label line 
		struct {			// local label id and direction (next/prev)
			uint8_t		local; 
			uint8_t		isnext;
		};
		uint32_t	ec;		// code emission line
	};

} label;


#if 0
#define LBL_PC( l )			{ .islocal = false, { .vline = (l) } }
#define LBL_NEXT( l )			{ .islocal = true,  { { .local = (l), .isnext = true } } }
#define LBL_PREV( l )			{ .islocal = true,  { { .local = (l), .isnext = false } } }
#else
static inline label LBL_PC( uint32_t line ){ label l = { .tag = LABEL_PC, { .vline = line } }; return l; }
static inline label LBL_NEXT( uint8_t local ){ label l = { .tag = LABEL_LOCAL }; l.local = local; l.isnext = true; return l; }
static inline label LBL_PREV( uint8_t local ){ label l = { .tag = LABEL_LOCAL }; l.local = local; l.isnext = false; return l; }
static inline label LBL_EC( unsigned int ec ){ label l = { .tag = LABEL_EC }; l.ec = ec; return l; }
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
