/*
* (C) Iain Fraser - GPLv3  
* Utility functions for mapping between different data spaces.
*/

#ifndef _MIPS_MAPPING_H_
#define _MIPS_MAPPING_H_

#include "math_util.h"
#include <assert.h>

#define NR_REGISTERS		18
#define NR_TAGS_IN_WORD		( ( sizeof( int ) * 8 ) / NR_TAG_BITS ) 	

#define TEMP_REG0	_a0
#define TEMP_REG1	_a1
#define TEMP_REG2	_a2
#define DATA_REG	_AT	// store pointer to data section

//int tag_count( int nr_locals );
int local_vreg_count( int nr_locals );
//int local_vreg( int local );
int total_vreg_count( int nr_locals, int nr_const );
int local_spill( int nr_locals );
int const_spill( int nr_locals, int nr_const );
int vreg_to_reg( int vreg );
void const_write_section( struct mips_emitter* me, int nr_locals );
void const_emit_loadreg( struct mips_emitter* me );

#endif
