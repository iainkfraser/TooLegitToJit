/*
* (C) Iain Fraser - GPLv3  
* MIPS opcodes manip
*/

#ifndef _ARCH_MIPS_OPCODES_H_
#define _ARCH_MIPS_OPCODES_H_

/*
* MIPs Opcodes [see Mips Run p 235]
*/

#define	MIPS_OPCODE(x)			( ( x )  >> 26  )
#define MIPS_SUBOPCODE_SPECIAL(x)	( ( x ) & 0x3f )	
#define MIPS_OPCODE_DSTREG(x)		( ( x >> 16 ) & 0x1f )
#define MIPS_OPCODE_SRCREG(x)		( ( x >> 11 ) & 0x1f ) 


#define GEN_MIPS_OPCODE_3REG( primaryop, rs, rt, rd, secondaryop )	\
	(								\
		( ( primaryop & 0x3f ) << 26 )  			\
			|						\
		( ( secondaryop & 0x3f ) )				\
			|						\
		( ( rs & 0x1f ) << 21 )					\
			|						\
		( ( rt & 0x1f ) << 16 )					\
			|						\
		( ( rd & 0x1f ) << 11 )					\
	)

#define GEN_MIPS_OPCODE_2REG( primaryop, rs, rt, immed )		\
	(								\
		( ( primaryop & 0x3f ) << 26 )  			\
			|						\
		( ( rs & 0x1f ) << 21 )					\
			|						\
		( ( rt & 0x1f ) << 16 )					\
			|						\
		(  immed & 0xffff )					\
	)

#define GEN_MIPS_OPCODE_SYSCALL( code )					\
	(								\
		( ( code & 0xfffff ) << 6 )				\
			|						\
		( 	12		)				\
	)

#define GEN_MIPS_OPCODE_3REG_TERT( primaryop, rs, rt, rd, secondaryop, tertop )	\
	( GEN_MIPS_OPCODE_3REG( primaryop, rs, rt, rd, secondaryop ) | ( ( tertop & 0x3f ) << 6 ) )

#define MOP_SPECIAL		0
#define		MOP_SPEICAL_SLL		0
#define		MOP_SPECIAL_ROTR	2
#define		MOP_SPECIAL_SLLV	4
#define 	MOP_SPECIAL_SRLV	6
#define 	MOP_SPECIAL_JR		8
#define 	MOP_SPECIAL_JALR	9
#define		MOP_SPECIAL_BREAK	13
#define		MOP_SPECIAL_MFHI	16	
#define		MOP_SPECIAL_MFLO	18
#define		MOP_SPECIAL_DIV		26	
#define 	MOP_SPEICAL_DIVU	27
#define		MOP_SPECIAL_ADD		32	
#define		MOP_SPECIAL_ADDU	33
#define		MOP_SPECIAL_SUB		34
#define		MOP_SPECIAL_SUBU	35
#define		MOP_SPECIAL_AND		36
#define		MOP_SPECIAL_OR		37
#define		MOP_SPECIAL_NOR		39

#define MOP_COND_BRANCH_OP1	1
#define MOP_BLTZ		1
#define		MOP_BLTZ_RT			0
#define MOP_BGEZ		1	
#define		MOP_BGEZ_RT			1			
#define		MOP_COND_BRANCH_OP1_TRAP_START	8
#define		MOP_COND_BRANCH_OP1_TRAP_END	14
#define	MOP_J			2
#define	MOP_JAL			3
#define MOP_BEQ			4
#define	MOP_BNE			5
#define MOP_BLEZ		6
#define MOP_BGTZ		7
#define MOP_ADDI		8
#define MOP_ADDIU		9
#define MOP_ORI			13
#define MOP_LUI			15
#define MOP_BEQL		20
#define MOP_BNEL		21
#define MOP_BLEZL		22
#define MOP_BGTZL		23
#define MOP_SPECIAL2		28
#define		MOP_SPECIAL2_MUL	2
#define MOP_SPECIAL3		31
#define 	MOP_SPECIAL3_BSHFL	32
#define			MOP_SPECIAL3_BSHFL_WSBH		2
#define		MOP_SPECIAL3_RDHWR	59
#define MOP_LB			32
#define MOP_LH			33
#define MOP_LW			35
#define MOP_LBU			36
#define MOP_LHU			37
#define MOP_LWU			39
#define MOP_SB			40
#define MOP_SH			41
#define MOP_SW			43

#define MOP_NOP			0


/*
* 1:1 macro to mips instruction  
*/

#define MI_ADDIU( rt, rs, i )	GEN_MIPS_OPCODE_2REG( MOP_ADDIU, rs, rt, (int16_t)(i) ) 
#define MI_ADDU( rd, rs, rt )	GEN_MIPS_OPCODE_3REG( MOP_SPECIAL, rs, rt, rd, MOP_SPECIAL_ADDU ) 
#define MI_B( off )		GEN_MIPS_OPCODE_2REG( MOP_BEQ, 0, 0, off )
#define MI_BEQ( rs, rt, off )	GEN_MIPS_OPCODE_2REG( MOP_BEQ, rs, rt, off )
#define MI_BGEZ( rs, off )	GEN_MIPS_OPCODE_2REG( 1, rs, MOP_BGEZ, off ) 
#define MI_BGTZ( rs, off )	GEN_MIPS_OPCODE_2REG( MOP_BGTZ, rs, 0, off )
#define MI_JALR( rs )		GEN_MIPS_OPCODE_3REG( MOP_SPECIAL, rs, _zero, _ra, MOP_SPECIAL_JALR )
#define MI_JR( rs )		GEN_MIPS_OPCODE_3REG( MOP_SPECIAL, rs, 0, 0, MOP_SPECIAL_JR )
#define MI_LW( rt, base, off )	GEN_MIPS_OPCODE_2REG( MOP_LW, base, rt, (int16_t)(off) )
#define MI_NOP()		MOP_NOP
#define MI_NOR( rd, rs, rt )	GEN_MIPS_OPCODE_3REG( MOP_SPECIAL, rs, rt, rd, MOP_SPECIAL_NOR ) 
#define MI_NOT( rd, rs )	MI_NOR( rd, rs, _zero ) 
#define MI_SLL( rd, rt, sa )	GEN_MIPS_OPCODE_3REG_TERT( MOP_SPECIAL, _zero, rt, rd, MOP_SPEICAL_SLL, sa ) 
#define MI_SUBU( rd, rs, rt ) 	GEN_MIPS_OPCODE_3REG( MOP_SPECIAL, rs, rt, rd, MOP_SPECIAL_SUBU ) 
#define MI_SW( rt, base, off )	GEN_MIPS_OPCODE_2REG( MOP_SW, base, rt, (int16_t)(off) )


/*
* Synthetic instructions 
*/
#define MI_NEGU( rd, rs )	MI_SUBU( rd, _zero, rs )


#endif
