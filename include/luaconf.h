/*
* (C) Iain Fraser - GPLv3 ( mostly Lua code so MIT if you want for this file ).
* Configure Lua. 
*/

#ifndef _LUACONF_H_
#define _LUACONF_H_

/*
@@ LUA_INT32 is an signed integer with exactly 32 bits.
@@ LUAI_UMEM is an unsigned integer big enough to count the total
@* memory used by Lua.
@@ LUAI_MEM is a signed integer big enough to count the total memory
@* used by Lua.
** CHANGE here if for some weird reason the default definitions are not
** good enough for your machine. Probably you do not need to change
** this.
*/
#if LUAI_BITSINT >= 32		/* { */
#define LUA_INT32	int
#define LUAI_UMEM	size_t
#define LUAI_MEM	ptrdiff_t
#else				/* }{ */
/* 16-bit ints */
#define LUA_INT32	long
#define LUAI_UMEM	unsigned long
#define LUAI_MEM	long
#endif				/* } */

/*
* LUA_API marks core functions. Can change dllexport for e.g. 
*/
#define LUA_API		extern

/*
* Do not change, integer only
*/
#define LUA_NUMBER	long

/*
@@ LUA_INTEGER is the integral type used by lua_pushinteger/lua_tointeger.
** CHANGE that if ptrdiff_t is not adequate on your machine. (On most
** machines, ptrdiff_t gives a good choice between int or long.)
*/
#define LUA_INTEGER	ptrdiff_t

/*
@@ LUA_UNSIGNED is the integral type used by lua_pushunsigned/lua_tounsigned.
** It must have at least 32 bits.
*/
#define LUA_UNSIGNED	unsigned LUA_INT32


/*
* LUA_PTR is integral type used by internal C call functions. Must be same
* size as register.
*/ 
#define LUA_PTR		uintptr_t

#endif

