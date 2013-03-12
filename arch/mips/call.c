/*
* (C) Iain Fraser - GPLv3  
* MIPS call instructions implementation. 
*/

#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include "machine.h"
#include "macros.h"
#include "emitter32.h"
#include "bit_manip.h"
#include "frame.h"
#include "jitfunc.h"
#include "arch/mips/machine.h"
#include "arch/mips/regdef.h"
#include "arch/mips/opcodes.h"
#include "arch/mips/arithmetic.h"
#include "arch/mips/call.h"
#include "arch/mips/branch.h"
#include "arch/mips/jfunc.h"

// use the sp variable in machine struct 
#undef sp
#undef fp


/*
* TODO: write up in machine spec that only b and call need to do absolute addressing.
*/

static inline void call_prologue( struct emitter* me, struct machine* m ){
#if 0 
	EMIT( MI_SW( _ra, m->sp, -4 ) );		// DO NOT PUT in delay slot because ra already overwritten.
#endif
}

static inline void call_epilogue( struct emitter* me, struct machine* m ){
#if 0
	EMIT( MI_ADDIU( m->sp, m->sp, -4 ) );	// delay slot
	EMIT( MI_LW( _ra, m->sp, 0 ) );
	EMIT( MI_ADDIU( m->sp, m->sp, 4 ) );
#else
	EMIT( MI_NOP() );
#endif
}


static void safe_direct_call( struct emitter* me, struct machine* m, label l ){
	assert( ISL_ABSDIRECT( l ) );
	
	uintptr_t pc = me->ops->absc( me );

	if( BITFIELD_DECODE( 28, 4, l.abs.k ) == BITFIELD_DECODE( 28, 4, pc ) ){ 
		call_prologue( me, m );
		EMIT( MI_JAL( l.abs.k ) );
		call_epilogue( me, m );	
	} else {
		operand jr = OP_TARGETREG( acquire_temp( _MOP, me, m ) );
		_MOP->move( me, m, jr, l.abs );
		call_prologue( me, m );
		EMIT( MI_JALR( jr.reg ) );
		RELEASE_OR_NOP( me, m );
		call_epilogue( me, m );	
	}
}

void mips_call( struct emitter* me, struct machine* m, label l ){
	operand fn;
	int temps;	
	
	if( ISL_ABS( l ) ){
		if( ISL_ABSDIRECT( l ) ){
			safe_direct_call( me, m, l );
		} else { 
			fn = l.abs;
			temps = load_coregisters( _MOP, me, m, 1, &fn );
			call_prologue( me, m );
			EMIT( MI_JALR( fn.reg ) );
			call_epilogue( me, m );
			unload_coregisters( _MOP, me, m, temps );
		}
	} else {
		call_prologue( me, m );
		EMIT( MI_BAL( mips_branch( me, l ) ) );
		call_epilogue( me, m );
	}

}

void mips_ret( struct emitter* me, struct machine* m ){
	EMIT( MI_JR( _ra ) );
	EMIT( MI_NOP( ) );
}



/*
* Calling C functions ( static and dynamic ).
*/

static int zerorow( const int n, int r, uint8_t dgraph[n][n] ){
	for( int j = 0; j < n; j++ ){
		if( dgraph[r][j] )
			return j + 1;
	}

	return 0;
}

static bool zerocol( const int n, int c, uint8_t dgraph[n][n] ){
	for( int i = 0; i < n; i++ ){
		if( i == c )	// ignore self ref which indicates stack dependency 
			continue;

		if( dgraph[i][c] )
			return false;
	}

	return true; 
}

static void resolv_cycle( struct emitter* me, struct frame* f, const int n, uint8_t dgraph[n][n], int v, int* sp ){
	// store vertex on stack
	*sp -= 4;

	for( int i = 0; i < n; i++ ){
		if( zerorow( n, i, dgraph ) == v ){
			dgraph[i][v-1] = 0;
			dgraph[i][i] = *sp;	// indicate that it depends on stack value	
		}
	}
}

/* resolve argument registers dependcies ( using stack if needbe ) */
static void assign_arg_regs( struct emitter* me, struct frame* f, const int n, uint8_t dgraph[n][n] ){
	int unresolved,resolved, v, stack = -4;
	operand dst, src;

	do{
		unresolved = resolved = 0;	

		for( int i = 0; i < n; i++ ){
			dst = OP_TARGETREG( _a0 + i );

			// does vertex i depend on ( at most ) 1 vertex?
			if( ( v = zerorow( n, i, dgraph ) ) ){	
				--v;	// zero based idx

				// do other vertices ( 1 or more ) depend on i?
				if( zerocol( n, i, dgraph ) ){		
					src =  v == i ?
						OP_TARGETDADDR( f->m->sp, dgraph[i][i] ) :
						OP_TARGETREG( _a0 + v );
	
					_MOP->move( me, f->m, dst, src );
					dgraph[i][v] = 0;
					resolved++;
				} else {
					unresolved = i + 1;
				}
			}	
		}

		// is there a cycle? 
		if( unresolved && !resolved )
			resolv_cycle( me, f, n, dgraph, unresolved, &stack );				
	}while( unresolved ); 
}

void mips_static_ccall( struct emitter* me, struct frame* f, uintptr_t fn, const operand* r, size_t argsz, ... ){
	assert( !has_spill( f->m ) );	// this fn call will clobber the crap out spilled regs

	va_list ap;
	va_start( ap, argsz );

	const int args_by_reg = 4;
	operand args[ argsz ];
	uint8_t dep_graph[ args_by_reg ][ args_by_reg ];
	
	for( int i = 0; i < argsz; i++ ){
		args[ i ] = va_arg( ap, operand );	
	}

	va_end( ap );

	// store all stack based args first, becase we may clobber src temps later.
	operand dst = OP_TARGETDADDR( f->m->sp, -4 );
	for( int i = args_by_reg; i < argsz; i++, dst.offset -= 4 ){
		_MOP->move( me, f->m, dst, args[i] );	
	}

	// generate single edge dependency matrix and load args that aren't dependent 
	memset( dep_graph, 0, sizeof( dep_graph ) );
	for( int i = 0; i < min( argsz, args_by_reg ); i++ ){
		if( ISO_REG( args[i] ) && MIPSREG_ISARG( args[i].reg ) )
			dep_graph[i][ MIPSREG_ARGIDX( args[i].reg ) ] = 1;
			
	}

	int counter = 0;
	while( temps_accessed( f->m ) < min( args_by_reg, argsz ) ){
		acquire_temp( _MOP, me, f->m );
		counter++;
	} 

	// move to correct arg registers - TODO: can clobber stack, there need to update stack before this??  
	assign_arg_regs( me, f, args_by_reg, dep_graph );

	// now depedency conflicts have been resolved load nonreg dependecies
	for( int i = 0; i < min( argsz, args_by_reg ); i++ ){
		if( !( ISO_REG( args[i] ) && MIPSREG_ISARG( args[i].reg ) ) )
			_MOP->move( me, f->m, OP_TARGETREG( _a0 + i ), args[i] );
	} 

	// spill MIPS temps
	jfunc_call( _MOP, me, f->m, jf_arch_idx( MJF_STORETEMP ), mjf_storetemp_offset( f->m, f->nr_locals ), JFUNC_UNLIMITED_STACK, 0 );

	const operand sp = OP_TARGETREG( f->m->sp );
	const operand ds = OP_TARGETIMMED( 4 * min( 4, argsz ) );
	// update stack and call	
	_MOP->sub( me, f->m, sp, sp, ds );
	_MOP->call( me, f->m, LBL_ABS( OP_TARGETIMMED( fn ) ) );	// TODO: do explcit call so that put sub into delay slot	
	_MOP->add( me, f->m, sp, sp, ds  );

	release_tempn( _MOP, me, f->m, counter );


	// fill MIPS temps
	jfunc_call( _MOP, me, f->m, jf_arch_idx( MJF_LOADTEMP ), mjf_loadtemp_offset( f->m, f->nr_locals ), JFUNC_UNLIMITED_STACK, 0 );

	if( r )
		_MOP->move( me, f->m, *r, OP_TARGETREG( _v0 ) );
}

