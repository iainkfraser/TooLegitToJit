/*
* (C) Iain Fraser - GPLv3  
* Bitwise manipulation
*/

#ifndef _BIT_MANIP_H_
#define _BIT_MANIP_H_

/*
* log2 of powers of 2
*/
#define	LOG2_1		0	
#define LOG2_2		1
#define LOG2_4		2
#define LOG2_8		3
#define LOG2_16		4
#define LOG2_32		5
#define LOG2_64		6
#define LOG2_128	7
#define LOG2_256	8
#define LOG2_512	9
#define LOG2_1024	10
#define LOG2_2048	11
#define LOG2_4096	12
#define LOG2_8192	13
#define LOG2_16384	14
#define LOG2_32768	15
#define LOG2_65536	16

/*
* Base 2 exponent and logs.
*/

#define __LOG2(x)	( LOG2_##x )

#define POWEROF2(x)	( 1 << x )
#define LOG2(x)		__LOG2(x)

/*
* Log2 on variables
*/

#ifdef _KERNEL

static inline unsigned int log2( unsigned int x ){
	unsigned int r = 0;
	while( x >>= 1 )
		r++; 

	return r;
}

#endif

/*
* Determine if value is power of 2
*/

#define ispowerof2(x)				\
	({					\
		typeof(x) a = ( x );		\
		!( a & ( a - 1 ) );		\
	})

/*
* Power of 2 rounding - TODO: use -y to do faster rounding
*/

#define roundup_power2(x,y)				\
	({						\
		typeof(x) a = ( x ); 			\
		typeof(y) b = ( y );			\
		assert( ispowerof2( b ) );		\
		b--;					\
		( a + b ) & ~b;				\
	})

#define rounddown_power2(x,y)				\
	({						\
		typeof(y) b = ( y );			\
		assert( ispowerof2( b ) );		\
		( x ) & ~( b - 1 );			\
	})


/*
* Bitfield encoding/decoding
*/ 

#define BIT(x)		( 1 << x )
#define BIT_GET(x,y)	( ( x ) & ( 1 << ( y ) ) )
#define BIT_SET(x,y)	do{ x |=  ( 1 << ( y ) ); }while(0)
#define BIT_CLEAR(x,y)  do{ x &= ~( 1 << ( y ) ); }while(0)

#define BITFIELD_MASK(size)	( POWEROF2(size) - 1 ) 

#define BITFIELD_ENCODE(startbit,size,value)	( ( value & BITFIELD_MASK( size ) ) << startbit )
#define BITFIELD_DECODE(startbit,size,value)	( ( value >> startbit ) & BITFIELD_MASK( size ) )

#define BITFIELD_WRITE(startbit,size,value,var)				\
	do{								\
		var |= 	BITFIELD_ENCODE(startbit,size,value);		\
	}while(0)

#define BITFIELD_OVERWRITE(startbit,size,value,var)					\
	do{										\
		var &= ~BITFIELD_ENCODE( startbit, size, BITFIELD_MASK(size) );		\
		BITFIELD_WRITE(startbit,size,value,var);				\
	}while(0)

/*
* fast divides,muls and moduli. Use indirection to 
* force prescan (i.e. all arguments get expanded once ) on
* strinfy / concantenation operator.
*/

#define __DIV_BY_POWEROF2(x,y)	( ( x ) >> LOG2_##y )
#define __MUL_BY_POWEROF2(x,y)	( ( x ) << LOG2_##y )

#define DIV_BY_POWEROF2(x,y)	__DIV_BY_POWEROF2( x ,y )
#define MUL_BY_POWEROF2(x,y)	__MUL_BY_POWEROF2( x ,y )
#define MOD_BY_POWEROF2(x,y)			\
	({					\
		typeof(y) b = ( y );		\
		assert( ispowerof2(b) );	\
		( x ) & ( b - 1 );			\
	})	



/* generate macros for reading and writing bitfields */
#define BITFIELD_ACCESS( prefix, startbit, size )			\
	static inline uint32_t prefix##_READ( uint32_t reg )		\
		{ return BITFIELD_DECODE( startbit, size, reg ); } 	\
 	static inline uint32_t prefix##_WRITE( uint32_t value )		\
		{ return BITFIELD_ENCODE( startbit, size, value ); }			

#endif
