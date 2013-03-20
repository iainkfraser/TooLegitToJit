/*
* (C) Iain Fraser - GPLv3  
* Util functions that suprisingly are not part of std C.
*/

#ifndef _MATH_UTIL_H_
#define _MATH_UTIL_H_

#define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })


#define bound( a, x, c )	\
	( max( min( x, c ), a ) ) 

#define int_ceil_div( a,  b )	\
	( ( ( a ) + ( ( b ) - 1 ) ) / ( b )  )

#define swap(a, b) \
         do { typeof(a) __tmp = (a); (a) = (b); (b) = __tmp; } while (0)

#define safe_free( x )	do{ if( (x) ) free( (x) ); } while( 0 )

#define static_assert(x)        do{switch(0){case false: case x:;}}while(0)

#define array_count( x )	( sizeof( x ) / sizeof( 0[x] ) )

/*
* Credit to Pádraig Brady, the below macros have his copyright. 
*/
#define ASSERT_CONCAT_(a, b) a##b
#define ASSERT_CONCAT(a, b) ASSERT_CONCAT_(a, b)
#define gl_static_assert(e)	enum { ASSERT_CONCAT(assert_line_, __LINE__) = 1/(!!(e)) } 


#endif

