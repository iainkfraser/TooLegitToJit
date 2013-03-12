/*
* (C) Iain Fraser - GPLv3 
* obj dump, for easy JIT'd code analysis. 
*/

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "elf.h"
#include "jitfunc.h" 
#include "machine.h"
#include "instruction.h"

#ifndef _ELFDUMP_
#error "ELF dump not defined"
#endif


static int fputns( FILE* f, const char* str ){
	int off = fprintf( f, "%s", str );
	off += fwrite( "\0", 1, 1, f );
	return off;
}

static void dump_section( FILE* f, Elf32_Word name, Elf32_Word type, Elf32_Word flags, Elf32_Word addr, Elf32_Word off,
					Elf32_Word size, Elf32_Word link, Elf32_Word info, Elf32_Word esize ){
	Elf32_Shdr entry = {
			.sh_name = name,
			.sh_type = type,
			.sh_flags = flags,
			.sh_addr = addr,
			.sh_offset = off,
			.sh_size = size,
			.sh_link = link,
			.sh_info = info,
			.sh_addralign = 0, 
			.sh_entsize = esize
	};

	fwrite( &entry, sizeof( entry ), 1, f );
}

static int dump_symbol( FILE* f, Elf32_Word name, Elf32_Word value, unsigned char info, Elf32_Half secidx ){
	Elf32_Sym s = {
		.st_name = name,
		.st_value = value,
		.st_size = 0,
		.st_info = info,
		.st_other = 0,
		.st_shndx = secidx	
	};

	fwrite( &s, sizeof( s ), 1, f );
	return 1;
}

static int dump_symbol_localfunc( FILE* f, Elf32_Word name, Elf32_Word value, Elf32_Half secidx ){
	return dump_symbol( f, name, value, ELF32_ST_INFO( STB_LOCAL, STT_FUNC ), secidx );
}


static void dump_elf_header( FILE* f, int nr_sections, int sechdroff, int strtabidx, unsigned char endian, Elf32_Half mach ){
	Elf32_Ehdr hdr = {
		.e_ident = { 
			0x7f, 'E', 'L', 'F', 
			[ EI_CLASS ] = EI_CLASS_32,
			[ EI_DATA ] = endian, 
			[ EI_VERSION ] = EV_CURRENT,
		}, 
		.e_type = ET_REL,
		.e_machine = mach,
		.e_version = EV_CURRENT,
		.e_entry = 0,
		.e_phoff = 0,
		.e_shoff = sechdroff,  
		.e_flags = 0,		// TODO: is this needed? 
		.e_ehsize = sizeof( Elf32_Ehdr ),
		.e_phentsize = sizeof( Elf32_Phdr ),
		.e_phnum = 0,
		.e_shentsize = sizeof( Elf32_Shdr ),
		.e_shnum = nr_sections,			 
		.e_shtrndx = strtabidx		
	};


	fwrite( &hdr, sizeof( hdr ), 1, f );
}

// dump Lua function label ( hirarchy based label )
static void dump_label( FILE* f, int n, int lbl[n], int *stidx ){
	*stidx += fprintf( f, "L" );

	for( int i = 0; i < n; i++ ){
		*stidx += fprintf( f, "%d", lbl[i] );
		if( i + 1 < n )
			*stidx += fputs( ".", f );
	}

	*stidx += fputns( f, "" );	
}

static void dump_stringtable_jfuncs( FILE* f, int *stidx, int archn ){
	for( int i = 0; i < JF_COUNT + archn; i++ ){
		jfuncs_get( i )->strtabidx = *stidx;
		*stidx += fprintf( f, "%s%d", i < JF_COUNT ? "J" : "JA", i < JF_COUNT ? i : i - JF_COUNT );
		*stidx += fputns( f, "" );	
	}
}

static int dump_symboltable_jfuncs( FILE* f, int secidx, struct emitter* e, int archn ){
	for( int i = 0; i < JF_COUNT + archn; i++ ){
		struct JFunc* j = jfuncs_get( i );

		dump_symbol_localfunc( f, j->strtabidx, jfunc_addr( e, i ) - jfunc_addr( e, 0 ), secidx );
	}

	return JF_COUNT;
}

static int dump_symboltable_protos( FILE* f, struct proto* p, int* secidx ){
	int count = 0;

	for( int i = 0; i < p->nrprotos; i++ ){
		count += dump_symboltable_protos( f, &p->subp[i], secidx );	
	}

	count += dump_symbol_localfunc( f, p->strtabidx, 0, (*secidx)++ );
	return count;
}

static void dump_stringtable_protos( FILE* f, struct proto* p, float idx, int n, int* src, int *stidx ){
	int label[ n + 1 ];

	// set label
	memcpy( label, src, sizeof( int ) * n );
	label[n] = idx;

	for( int i = 0; i < p->nrprotos; i++ ){
		dump_stringtable_protos( f, &p->subp[i], i, n+1, label, stidx );
	}

	// TODO: rememmeber index into string table
	p->strtabidx = *stidx;
	dump_label( f, n + 1, label, stidx );	

}

static long dump_section_jfuncs( FILE* f, void *jsection, size_t jsize ){
	long start = ftell( f );
	fwrite( jsection, jsize, 1, f );
	return start;
}

static void dump_section_protos( FILE* f, struct proto* p ){
	for( int i = 0; i < p->nrprotos; i++ ){
		dump_section_protos( f, &p->subp[i] );	
	}

	p->secoff = ftell( f );
	fwrite( p->code, p->sizemcode, 1, f );	
	p->secend = ftell( f );	
}

static void dump_sectionentry_protos( FILE* f, struct proto* p, int stridtext, int sytabidx ){
	for( int i = 0; i < p->nrprotos; i++ ){
		dump_sectionentry_protos( f, &p->subp[i], stridtext, sytabidx );
	}

	dump_section( f, stridtext, SHT_PROGBITS, SHF_ALLOC | SHF_EXECINSTR, 
			(intptr_t)p->code, p->secoff, p->secend - p->secoff, sytabidx, 0, 0 );

}

void serialise( struct proto* main, char* filepath, void *jsection, size_t jsize, struct machine* m, struct emitter* e, int archn ){
	int stridst, stridtext, stridsyt, stidx = 0, stoff, stend, syoff, syend, jfuncoff, sechdroff, sysz;
	FILE* o = fopen( filepath, "w" );

	const int strtabidx = 1;	// string table always after null entry in section header
	const int sytabidx = 2;
	int luasecstart = 4;
	int nr_lsections = 0;
	
	// skip over the header 
	fseek( o, sizeof( Elf32_Ehdr ), SEEK_CUR );
	
	// dump string table
	stoff = ftell( o );
	stridtext = ( stidx += fputns( o, "" ) );
	stridst =  ( stidx += fputns( o, ".text" ) );
	stridsyt = ( stidx += fputns( o, ".strtab" ) );
	stidx += fputns( o, ".symtab" );
	dump_stringtable_jfuncs( o, &stidx, archn );
	dump_stringtable_protos( o, main, 0, 0, NULL, &stidx );
	stend = ftell( o );

	// dump sections
	jfuncoff = dump_section_jfuncs( o, jsection, jsize );
	dump_section_protos( o, main );

	// dump symbol table
	sysz = 0;
	syoff = ftell( o );
	sysz += dump_symbol( o, 0, 0, 0, SHN_UNDEF );
	sysz += dump_symboltable_jfuncs( o, sytabidx + 1, e, archn );	// jfuncs section is after NULL, strtab and symtab
	sysz += ( nr_lsections = dump_symboltable_protos( o, main, &luasecstart ) );
	syend = ftell( o );

	// dump section header entries 
	sechdroff = ftell( o );
	dump_section( o, 0, SHT_NULL, 0, 0, 0, 0, SHN_UNDEF, 0, 0 );				// null section
	dump_section( o, stridst, SHT_STRTAB, 0, 0, stoff, stend - stoff, SHN_UNDEF, 0, 0 );	// string table
	dump_section( o, stridsyt, SHT_SYMTAB, 0, 0, syoff, syend - syoff, strtabidx, sysz, sizeof( Elf32_Sym ) );
	dump_section( o, stridtext, SHT_PROGBITS, SHF_ALLOC | SHF_EXECINSTR, (intptr_t)jsection, jfuncoff, jsize, sytabidx, 0, 0 );
	dump_sectionentry_protos( o, main, stridtext, sytabidx );
	
	// goto start and dump header
	fseek( o, 0, SEEK_SET );
	dump_elf_header( o, 4 + nr_lsections, sechdroff, strtabidx, m->elf_endian, m->elf_machine );
	
	fclose( o );
}
