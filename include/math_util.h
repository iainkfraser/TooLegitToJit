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
#endif

