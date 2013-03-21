/*
* (C) Iain Fraser - GPLv3 
* Lua C API
*/

#ifndef _LAPI_H_
#define _LAPI_H_

#include <stdarg.h>
#include "luaconf.h"

/*
** pseudo-indices
*/
#define LUA_REGISTRYINDEX	LUAI_FIRSTPSEUDOIDX
#define lua_upvalueindex(i)	(LUA_REGISTRYINDEX - (i))

/* minimum Lua stack available to a C function */
#define LUA_MINSTACK	20

//struct lua_State;
typedef struct lua_State lua_State;

/* type of numbers in Lua */
typedef LUA_NUMBER lua_Number;

/* type for integer functions */
typedef LUA_INTEGER lua_Integer;

/* unsigned integer type */
typedef LUA_UNSIGNED lua_Unsigned;

typedef int (*lua_CFunction) ( struct lua_State *L );

struct TValue;

/*
* Useful internal stack manipulation. Internal code
* is adviced to *always* use these incase the stack
* implementation changes. 
*
* TODO: Make sure not in the api header i.e. lua.h
*/
struct TValue lua_safepop( lua_State *L );
void lua_safepush( lua_State *L, struct TValue v );
struct TValue* index2addr (lua_State *L, int idx);
void initstack( lua_State *L, struct TValue* s, size_t sz );

/*
** basic stack manipulation
*/
LUA_API int   (lua_absindex) (lua_State *L, int idx);
LUA_API int   (lua_gettop) (lua_State *L);
LUA_API void  (lua_settop) (lua_State *L, int idx);
LUA_API void  (lua_pushvalue) (lua_State *L, int idx);
LUA_API void  (lua_remove) (lua_State *L, int idx);
LUA_API void  (lua_insert) (lua_State *L, int idx);
LUA_API void  (lua_replace) (lua_State *L, int idx);
LUA_API void  (lua_copy) (lua_State *L, int fromidx, int toidx);
LUA_API int   (lua_checkstack) (lua_State *L, int sz);

LUA_API void  (lua_xmove) (lua_State *from, lua_State *to, int n);


/*
** push functions (C -> stack)
*/
LUA_API void        (lua_pushnil) (lua_State *L);
LUA_API void        (lua_pushnumber) (lua_State *L, lua_Number n);
LUA_API void        (lua_pushinteger) (lua_State *L, lua_Integer n);
LUA_API void        (lua_pushunsigned) (lua_State *L, lua_Unsigned n);
LUA_API const char *(lua_pushlstring) (lua_State *L, const char *s, size_t l);
LUA_API const char *(lua_pushstring) (lua_State *L, const char *s);
LUA_API const char *(lua_pushvfstring) (lua_State *L, const char *fmt,
                                                      va_list argp);
LUA_API const char *(lua_pushfstring) (lua_State *L, const char *fmt, ...);
LUA_API void  (lua_pushcclosure) (lua_State *L, lua_CFunction fn, int n);
LUA_API void  (lua_pushboolean) (lua_State *L, int b);
LUA_API void  (lua_pushlightuserdata) (lua_State *L, void *p);
LUA_API int   (lua_pushthread) (lua_State *L);

#define lua_pushcfunction(L,f)	lua_pushcclosure(L, (f), 0)

/*
** get functions (Lua -> stack)
*/
LUA_API void  (lua_getglobal) (lua_State *L, const char *var);
LUA_API void  (lua_gettable) (lua_State *L, int idx);
LUA_API void  (lua_getfield) (lua_State *L, int idx, const char *k);
LUA_API void  (lua_rawget) (lua_State *L, int idx);
LUA_API void  (lua_rawgeti) (lua_State *L, int idx, int n);
LUA_API void  (lua_rawgetp) (lua_State *L, int idx, const void *p);
LUA_API void  (lua_createtable) (lua_State *L, int narr, int nrec);
LUA_API void *(lua_newuserdata) (lua_State *L, size_t sz);
LUA_API int   (lua_getmetatable) (lua_State *L, int objindex);
LUA_API void  (lua_getuservalue) (lua_State *L, int idx);


/*
** set functions (stack -> Lua)
*/
LUA_API void  (lua_setglobal) (lua_State *L, const char *var);
LUA_API void  (lua_settable) (lua_State *L, int idx);
LUA_API void  (lua_setfield) (lua_State *L, int idx, const char *k);
LUA_API void  (lua_rawset) (lua_State *L, int idx);
LUA_API void  (lua_rawseti) (lua_State *L, int idx, int n);
LUA_API void  (lua_rawsetp) (lua_State *L, int idx, const void *p);
LUA_API int   (lua_setmetatable) (lua_State *L, int objindex);
LUA_API void  (lua_setuservalue) (lua_State *L, int idx);

/*
** 'load' and 'call' functions (load and run Lua code)
*/

void lua_call (lua_State *L, int nargs, int nresults);
int lua_pcall (lua_State *L, int nargs, int nresults, int msgh);

/*
** ===============================================================
** some useful macros
** ===============================================================
*/

#define lua_pop(L,n)		lua_settop(L, -(n)-1)

#endif
