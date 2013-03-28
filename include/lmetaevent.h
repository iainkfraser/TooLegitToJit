/*
* (C) Iain Fraser - GPLv3 
*
* Handle events using values metatable as described in reference man.
*/

#ifndef _LMETAEVENT_H_
#define _LMETAEVENT_H_

#include <stdbool.h>

typedef enum {
  TM_INDEX,
  TM_NEWINDEX,
  TM_GC,
  TM_MODE,
  TM_LEN,
  TM_EQ,  /* last tag method with `fast' access */
  TM_ADD,
  TM_SUB,
  TM_MUL,
  TM_DIV,
  TM_MOD,
  TM_POW,
  TM_UNM,
  TM_LT,
  TM_LE,
  TM_CONCAT,
  TM_CALL,
  TM_N		/* number of elements in the enum */
} TMS;


struct TValue* getmt( struct TValue* t );		// get assoc metatable
struct TValue* gettm( struct TValue* t, const TMS e );	// get assoc tag method


bool mt_call_bintm( struct TValue* tm, struct TValue* s, struct TValue* t
						, struct TValue* d );
bool mt_call_binmevent( struct TValue* mt, struct TValue* s, struct TValue* t
					, struct TValue* d, int event );
bool call_binmevent( struct TValue* s, struct TValue* t, struct TValue* d
							, int event );

#endif
