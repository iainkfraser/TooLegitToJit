/*
* (C) Iain Fraser - GPLv3 
*
* Platfrom indepdent vm and physical machine operand mapping.
*/

#ifndef _OPERAND_H_
#define _OPERAND_H_

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
* Generic machine Lua value 
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
