/*
* (C) Iain Fraser - GPLv3  
* Physical machine interface. Utility functions. 
*/

#include <stdarg.h>
#include <stdbool.h>
#include "machine.h"

#define REG_MASK		0xffff
#define GET_REG( r ) 		( ( r ) & REG_MASK )
#define GET_REFCOUNT( r )	( ( ( r ) >> 16 ) & REG_MASK )
#define SET_REFCOUNT( r, rc )	( ( ( ( rc ) & 0xffff ) << 16 ) | GET_REG( r )  )
#define STACK_POS( idx, rc, n, wordsize )		\
	( 4 * ( ( rc - 1 ) * n  + idx + 1 )

static int abs_stack_offset( int idx, int rc, int n, size_t word ){
	return 4 * ( ( rc  - 1 ) * n + idx + rc );
}

int acquire_temp( struct machine_ops* mop, struct emitter* e, struct machine* m ) {
	int i,rc,reg;

	// slots refcount have invariant slot( i + 1 ) <= slot( i ),  i >= 0 
	for( i = m->nr_temp_regs - 1; i > 0; i-- ){
		if( GET_REFCOUNT( m->reg[i] ) < GET_REFCOUNT( m->reg[i-1] ) )
			break;	
	}

	rc = GET_REFCOUNT( m->reg[i] );
	reg = GET_REG( m->reg[i] );

	// if being accessed already then spill onto the stack 
	if( rc > 0 ){
		assert( m->allow_spill );	// TODO: error handling 
		
		operand dst = OP_TARGETDADDR( m->sp, -abs_stack_offset( i, rc, m->nr_temp_regs, 4 ) );
		operand src = OP_TARGETREG( reg ); 
		
		bool prior = disable_spill( m );
		mop->move( e, m, dst, src ); 		
		restore_spill( m, prior );
	 

	}

	m->reg[i] = SET_REFCOUNT( m->reg[i], ++rc );	

	// update max
	int ta = temps_accessed( m );
	if( ta > m->max_access )
		m->max_access = ta;

	return reg; 
}

bool release_temp( struct machine_ops* mop, struct emitter* e, struct machine* m ) {
	int i,rc, reg;

	for( i = 0; i < m->nr_temp_regs - 1; i++ ){
 		if( GET_REFCOUNT( m->reg[i] ) > GET_REFCOUNT( m->reg[i+1] ) )
			break;
	}

	reg = GET_REG( m->reg[i] );
	rc = GET_REFCOUNT( m->reg[i] );
	assert( rc > 0 );

	// if after release its still being accessed then reload from stack  
	if( --rc > 0 ){
		operand dst = OP_TARGETREG( reg ); 
		operand src = OP_TARGETDADDR( m->sp, -abs_stack_offset( i, rc, m->nr_temp_regs, 4 ) );
		
		bool prior = disable_spill( m );
		mop->move( e, m, dst, src ); 		
		restore_spill( m, prior );
	}

	m->reg[i] = SET_REFCOUNT( m->reg[i], rc );

	return rc > 0;	
}


int temps_accessed( struct machine* m ){
	int i;

	for( i = 0; i < m->nr_temp_regs - 1; i++ ){
 		if( GET_REFCOUNT( m->reg[i] ) > GET_REFCOUNT( m->reg[i+1] ) )
			break;
	}

	return ( GET_REFCOUNT( m->reg[i] ) - 1 ) * m->nr_temp_regs + i + 1;
}


void release_tempn( struct machine_ops* mop, struct emitter* e, struct machine* m, int n ){
	for( int i = 0; i < n; i++ )
		release_temp( mop, e, m );
}

bool is_temp( struct machine* m, int reg ){
	for( int i = 0; i < m->nr_temp_regs; i++ ){
		if( GET_REG( m->reg[i] ) == reg )
			return true;
	}
	
	return false;
}

/*
* Spill toggle 
*/

bool disable_spill( struct machine* m ){
	bool prior = m->allow_spill;
	m->allow_spill = false;
	return prior; 
}

void restore_spill( struct machine* m, bool prior ){
	m->allow_spill = prior; 
}

void enable_spill( struct machine* m ){
	m->allow_spill = true; 
}


/*
* Simultaneous register loads. 
*/

int load_coregisters( struct machine_ops* mop, struct emitter* me, struct machine* m, int nr_oper, ... ){
	va_list ap;
	operand *op;
	int temps = 0;

	va_start( ap, nr_oper );

	for( int i = 0; i < nr_oper; i++ ){
		op = va_arg( ap, operand* );
		if( op->tag != OT_REG ){
			operand dst = OP_TARGETREG( acquire_temp( mop, me, m ) ); 
		
			bool prior = disable_spill( m );
			mop->move( me, m, dst, *op ); 		
			restore_spill( m, prior );

			*op = dst; 
			temps++;
		}
	}

	va_end( ap );

	// verify simultenously in registers
	assert( temps <= m->nr_temp_regs );

	return temps;
}

void unload_coregisters( struct machine_ops* mop, struct emitter* me, struct machine* m, int n ){
	for( int i = 0; i < n; i++)
		release_temp( mop, me, m );
}

