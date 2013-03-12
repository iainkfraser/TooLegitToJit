/*
* (C) Iain Fraser - GPLv3 
* obj dump, for easy JIT'd code analysis. 
*/

#ifndef _ELF_H_
#define _ELF_H_

#include <stdint.h>

struct proto;
struct machine;
struct emitter;
void serialise( struct proto* main, char* filepath, void *jsection, size_t jsize, struct machine* m, struct emitter* e, int archn );

#define ELF_MAGICSIZE		4

/* header magic */
#define EI_CLASS	4		// class of file i.e. 32bit,64-bit, etc
#define		EI_CLASS_NONE		0
#define		EI_CLASS_32		1
#define		EI_CLASS_64		2
#define EI_DATA		5		// data encoding ( endianess )
#define		EI_DATA_NONE		0
#define		EI_DATA_LSB		1	// little endian
#define		EI_DATA_MSB		2	// big endian
#define EI_VERSION	6		// file version
#define EI_PAD		7		// start of padding bytes i.e. till EI_NIDENT can be ignored
#define EI_NIDENT 		16

/* header version */
#define EV_NONE		0
#define EV_CURRENT	1

/* header type */
#define ET_NONE		0
#define ET_REL		1	// relocatable file
#define ET_EXEC		2	// executable file
#define ET_DYN		3	// shared obj file
#define ET_CORE		4	// core file
#define ET_LOPROC	0xff00	// start of specialist file region
#define ET_HIPROC	0xffff	// end of specialist file region

/* header machine */
#define EM_NONE		0
#define EM_386		3	// intel 80386
#define EM_MIPS		8	// mips RS3000

/* page header entry types */
#define PT_NULL		0
#define PT_LOAD		1

/* page header entry flags */
#define PF_X		0x1
#define PF_W		0x2
#define PF_R		0x4

/* section header types */
#define SHT_NULL	0
#define SHT_PROGBITS	1
#define SHT_SYMTAB	2
#define SHT_STRTAB	3

/* section header special index's */
#define SHN_UNDEF	0

/* section header flags */
#define SHF_ALLOC	0x2
#define SHF_EXECINSTR	0x4

/* Symbol info access */
#define ELF32_ST_BIND(i)	((i)>>4)
#define ELF32_ST_TYPE(i)	((i)&0xf)
#define ELF32_ST_INFO(b,t)	(((b)<<4)+((t)&0xf))

/* Symbol binds */
#define STB_LOCAL	0

/* Symbol types */
#define STT_FUNC 	2

/*
* Elf control data is machine independent and use
* the following types sizes. The byte-order is determined
* by reading the ident bytes in the header ( it should
* be byte order of target ).
*/
typedef uint32_t	Elf32_Addr;
typedef uint16_t	Elf32_Half;
typedef uint32_t	Elf32_Off;
typedef uint32_t	Elf32_Sword;
typedef uint32_t	Elf32_Word;


typedef struct { 
	unsigned char	e_ident[EI_NIDENT];
	Elf32_Half 	e_type;
	Elf32_Half	e_machine;
	Elf32_Word	e_version;
	Elf32_Addr 	e_entry;
	Elf32_Off 	e_phoff;
	Elf32_Off 	e_shoff;
	Elf32_Word 	e_flags;
	Elf32_Half 	e_ehsize;
	Elf32_Half 	e_phentsize;
	Elf32_Half 	e_phnum;
	Elf32_Half 	e_shentsize;
	Elf32_Half 	e_shnum;
	Elf32_Half	e_shtrndx;
} Elf32_Ehdr; 


typedef struct {
	Elf32_Word	p_type;
	Elf32_Off	p_offset;
	Elf32_Addr	p_vaddr;
	Elf32_Addr	p_paddr;
	Elf32_Word	p_filesz;
	Elf32_Word	p_memsz;
	Elf32_Word	p_flags;
	Elf32_Word	p_align;
} Elf32_Phdr;


typedef struct {
	Elf32_Word sh_name;
	Elf32_Word sh_type;
	Elf32_Word sh_flags;
	Elf32_Addr sh_addr;
	Elf32_Off sh_offset;
	Elf32_Word sh_size;
	Elf32_Word sh_link;
	Elf32_Word sh_info;
	Elf32_Word sh_addralign;
	Elf32_Word sh_entsize;
} Elf32_Shdr;


typedef struct {
	Elf32_Word st_name;
	Elf32_Addr st_value;
	Elf32_Word st_size;
	unsigned char st_info;
	unsigned char st_other;
	Elf32_Half st_shndx;
} Elf32_Sym;



#endif
