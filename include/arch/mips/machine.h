/*
* (C) Iain Fraser - GPLv3  
* MIPs machine implementation. 
*/

#ifndef _ARCH_MIPS_MACHINE_H_
#define _ARCH_MIPS_MACHINE_H_

extern struct machine_ops mips_ops;

#define _MOP	( &mips_ops )
#define RELEASE_OR_NOP( me, m )				\
	do{						\
		if( !release_temp( _MOP, me, m ) )	\
			EMIT( MI_NOP() );		\
	}while( 0 )



void move( struct emitter* me, struct machine* m, operand d, operand s );
bool is_add_immed( operand o );
bool is_sub_immed( operand o );
bool is_mips_temp( operand o );

#endif
