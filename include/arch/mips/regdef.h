/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1994, 1995 by Ralf Baechle
 */

#ifndef __ASM_MIPS_REGDEF_H
#define __ASM_MIPS_REGDEF_H


#define _zero    0      /* wired zero */
#define _AT      1      /* assembler temp  - uppercase because of ".set at" */
#define _v0      2      /* return value */
#define _v1      3
#define _a0      4      /* argument registers */
#define _a1      5
#define _a2      6
#define _a3      7
#define _t0      8      /* caller saved */
#define _t1      9
#define _t2      10
#define _t3      11
#define _t4      12
#define _t5      13
#define _t6      14
#define _t7      15
#define _s0      16     /* callee saved */
#define _s1      17
#define _s2      18
#define _s3      19
#define _s4      20
#define _s5      21
#define _s6      22
#define _s7      23
#define _t8      24     /* caller saved */
#define _t9      25
#define _jp      25     /* PIC jump register */
#define _k0      26     /* kernel scratch */
#define _k1      27
#define _gp      28     /* global pointer */
#define _sp      29     /* stack pointer */
#define _fp      30     /* frame pointer */
#define _s8	30	/* same like fp! */
#define _ra      31     /* return address */

/*
 * Symbolic register names for 32 bit ABI
 */
#define zero    $0      /* wired zero */
#define AT      $1      /* assembler temp  - uppercase because of ".set at" */
#define v0      $2      /* return value */
#define v1      $3
#define a0      $4      /* argument registers */
#define a1      $5
#define a2      $6
#define a3      $7
#define t0      $8      /* caller saved */
#define t1      $9
#define t2      $10
#define t3      $11
#define t4      $12
#define t5      $13
#define t6      $14
#define t7      $15
#define s0      $16     /* callee saved */
#define s1      $17
#define s2      $18
#define s3      $19
#define s4      $20
#define s5      $21
#define s6      $22
#define s7      $23
#define t8      $24     /* caller saved */
#define t9      $25
#define jp      $25     /* PIC jump register */
#define k0      $26     /* kernel scratch */
#define k1      $27
#define gp      $28     /* global pointer */
#define sp      $29     /* stack pointer */
#define fp      $30     /* frame pointer */
#define s8	$30	/* same like fp! */
#define ra      $31     /* return address */


#define MIPSREG_ISTEMP( r )	( ( (r) >= _t0 && (r) <= _t7 ) || ( (r) >= _t8 && (r) <= _t9 ) )
#define MIPSREG_ISARG( r )	( (r) >= _a0 && (r) <= _a3 )
#define MIPSREG_ARGIDX( r )	( (r) - _a0 )

#endif /* __ASM_MIPS_REGDEF_H */
