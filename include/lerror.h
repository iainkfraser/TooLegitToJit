/*
* (C) Iain Fraser - GPLv3 
*
* Error codes and functions.
*/

#ifndef _LERROR_H_
#define _LERROR_H_

#define LE_ORDER	1	// relational error
#define LE_ARITH	2	// arithmetic error 
#define LE_TYPE		3	// type error 

void lerror( lua_State* L, int ecode );
void lcurrent_error( int ecode );

#endif
