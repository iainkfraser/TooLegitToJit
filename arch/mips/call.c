/*
* (C) Iain Fraser - GPLv3  
* MIPS call instructions implementation. 
*/

#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include "machine.h"
#include "macros.h"
#include "emitter32.h"
#include "bit_manip.h"
#include "frame.h"
#include "arch/mips/regdef.h"
#include "arch/mips/opcodes.h"
#include "arch/mips/arithmetic.h"
#include "arch/mips/call.h"
#include "arch/mips/branch.h"

// use the sp variable in machine struct 
#undef sp
#undef fp

extern struct machine_ops mips_ops;

#define _MOP	( &mips_ops )
#define RELEASE_OR_NOP( me, m )				\
	do{						\
		if( !release_temp( _MOP, me, m ) )	\
			EMIT( MI_NOP() );		\
	}while( 0 )

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

static bool require_spill( operand o ){
	return ISO_REG( o ) && MIPSREG_ISTEMP( o.reg );
}

static void do_spill( struct emitter* me, struct frame* f, operand a, operand b, bool doswap ){
	if( doswap )
		swap( a, b );

	_MOP->move( me, f->m, a, b );
}

static void tempreg_spill( struct emitter* me, struct frame* f, bool isstore ){

	for( int i = 0; i < f->nr_locals; i++ ){
		vreg_operand on = vreg_to_operand( f, i, false );
		vreg_operand off = vreg_to_operand( f, i, true );

		if( require_spill( on.value ) )
			do_spill( me, f, off.value, on.value, !isstore );

		if( require_spill( on.type ) )
			do_spill( me, f, off.type, on.type, !isstore );
	}
}

void mips_static_ccall( struct emitter* me, struct frame* f, uintptr_t fn, const operand* r, size_t argsz, ... ){
	assert( temps_accessed( f->m ) == 0 );	// temps == arg regs	
	va_list ap;
	va_start( ap, argsz );

	const int args_by_reg = 4;
	uint8_t dep_graph[ args_by_reg ][ args_by_reg ] = {};
	
	for( int i = 0; i < 4; i++ ){

	}

	va_end( ap );

	return;
	// TODO: make this check the temps are actually a0 
	for( int i = 0; i < min( 4, argsz ); i++ )
		acquire_temp( _MOP, me, f->m );	


	const int wordsz = 4;			// TODO: handle 64 bit
	const int n = max( args_by_reg, argsz );
	const int stack_delta = wordsz * n;
	operand sp = OP_TARGETREG( f->m->sp );
	operand ds = OP_TARGETIMMED( stack_delta );
	operand argreg = OP_TARGETREG( _a0 );
	operand argstk = OP_TARGETDADDR( f->m->sp, -4  );
	tempreg_spill( me, f, true );
	
	for( int i = 0; i < argsz; i++ ){
		if( i < 4 )
			_MOP->move( me, f->m, argreg, va_arg( ap, operand ) );	
		else
			_MOP->move( me, f->m, argstk, va_arg( ap, operand ) );	// save to stack

		// update reg and stack
		argreg.reg++;
		argstk.offset += wordsz;

	}

	va_end( ap );
	
	_MOP->sub( me, f->m, sp, sp, ds );
	_MOP->call( me, f->m, LBL_ABS( OP_TARGETIMMED( fn ) ) );	// TODO: do explcit call so that put sub into delay slot	
	_MOP->add( me, f->m, sp, sp, ds  );


	// reload temps	
	tempreg_spill( me, f, false );

	for( int i = 0; i < min( 4, argsz ); i++ )
		release_temp( _MOP, me, f->m );	

	if( r )
		_MOP->move( me, f->m, *r, OP_TARGETREG( _v0 ) );
#if 0
	// first 10 virtual regs mapped to temps so save them 
	int temps = min( 10, nr_livereg_vreg_occupy( me->nr_locals ) );
	for( int i=0; i < temps; i++ ){
		int treg = vreg_to_physical_reg( i );
		ENCODE_OP( me, GEN_MIPS_OPCODE_2REG( MOP_SW, _sp, treg, (int16_t)-( (i+1) * 4 ) ) );
	}
		
	// need space for temps and function arguments	
	int stackspace = 4 * ( max( argsz, 4 ) + temps );
	
	// create space for args - assume caller has setup a0 and a1
	ENCODE_OP( me, GEN_MIPS_OPCODE_2REG( MOP_ADDIU, _sp, _sp, (int16_t)( -stackspace ) ) );

	// can assume loading into _v0 is safe because function may overwrite it 
	loadim( me, _v0, fn );
	ENCODE_OP( me, GEN_MIPS_OPCODE_3REG( MOP_SPECIAL, _v0, _zero, _ra, MOP_SPECIAL_JALR ) );
	ENCODE_OP( me, MOP_NOP );	// TODO: one of the delay slots could be saved reg
	
	// restore the stack 	
	ENCODE_OP( me, GEN_MIPS_OPCODE_2REG( MOP_ADDIU, _sp, _sp, (int16_t)( stackspace ) ) );
	
	// reload temps
	for( int i=0; i < temps; i++ ){
		int treg = vreg_to_physical_reg( i );
		ENCODE_OP( me, GEN_MIPS_OPCODE_2REG( MOP_LW, _sp, treg, (int16_t)-( (i+1) * 4 ) ) );
	}
#endif 
}
