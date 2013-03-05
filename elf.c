/*
* (C) Iain Fraser - GPLv3 
* obj dump, for easy JIT'd code analysis. 
*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "elf.h"
#include "jitfunc.h" 

#ifndef _ELFDUMP_
#error "ELF dump not defined"
#endif


static int fputns( FILE* f, const char* str ){
	int off = fprintf( f, "%s", str );
	off += fwrite( "\0", 1, 1, f );
	return off;
}

static void dump_elf_header( FILE* f, int nr_sections, int sechdroff, int strtabidx ){
	Elf32_Ehdr hdr = {
		.e_ident = { 
			0x7f, 'E', 'L', 'F', 
			[ EI_CLASS ] = EI_CLASS_32,
			[ EI_DATA ] = EI_DATA_LSB, 
			[ EI_VERSION ] = EV_CURRENT,
		}, 
		.e_type = ET_REL,
		.e_machine = EM_MIPS,
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

static void dump_label( FILE* f, int n, int lbl[n], int *stidx ){
	for( int i = 0; i < n; i++ ){
		stidx += fprintf( f, "%d", lbl[i] );
		if( i + 1 < n )
			stidx += fputs( ".", f );
	}

	stidx += fputns( f, "" );	
}

static void dump_stringtable_jfuncs( FILE* f, int *stidx ){
	for( int i = 0; i < JF_COUNT; i++ ){
		jfuncs_get( i )->strtabidx = *stidx;
		*stidx += fprintf( f, "J%d", i );	
	}
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

}

static void dump_section( FILE* f, Elf32_Word name, Elf32_Word type, Elf32_Word flags, Elf32_Word addr, Elf32_Word off,
					Elf32_Word size, Elf32_Word link, Elf32_Word info  ){
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
			.sh_entsize = 0
	};

	fwrite( &entry, sizeof( entry ), 1, f );
}


static void dump_section_header( FILE* f, struct proto* p, int secst, int secjfuncs, int nrprotos, int txtidx ){
	Elf32_Shdr hdr[ JF_COUNT + 2 ] = {
		{
			.sh_name = 0,
			.sh_type = SHT_NULL,
			.sh_flags = 0,
			.sh_addr = 0,
			.sh_offset = 0,
			.sh_size = 0,
			.sh_link = SHN_UNDEF,
			.sh_info = 0,
			.sh_addralign = 0, 
			.sh_entsize = 0
		} 
	};
	
	for( int i = 1; i < JF_COUNT; i++ ){
		hdr[ i ].sh_name = 1;	// TODO: set string table index 1 to .text
		hdr[ i ].sh_type = SHT_PROGBITS;
		hdr[ i ].sh_flags = SHF_ALLOC | SHF_EXECINSTR;
		hdr[ i ].sh_addr = 0;		// TODO: the memory location of section
		hdr[ i ].sh_offset = 0;		// TODO: determine the location 
		hdr[ i ].sh_size = 0;		// TODO: determine size of section
		hdr[ i ].sh_link = 0;		// TODO: the section header index of the symbol table for this index
		hdr[ i ].sh_info = 0;		// TODO: the section header index of the section to which relo applies
		hdr[ i ].sh_addralign = 0;
		hdr[ i ].sh_entsize = 0; 
	}	

	fwrite( &hdr, sizeof( hdr ), 1, f );
}

void serialise( struct proto* main, char* filepath, void *jsection, size_t jsize ){
	int stridst, stridtext, stidx = 0, stoff, stend, jfuncoff, sechdroff;
	const int strtabidx = 1;	// string table always after null entry in section header
	FILE* o = fopen( filepath, "w" );

	// skip over the header 
	fseek( o, sizeof( Elf32_Ehdr ), SEEK_CUR );
	
	// dump string table
	stoff = ftell( o );
	stridtext = ( stidx += fputns( o, "" ) );
	stridst =  ( stidx += fputns( o, ".text" ) );
	( stidx += fputns( o, ".strtab" ) );
	dump_stringtable_jfuncs( o, &stidx );
	dump_stringtable_protos( o, main, 0, 0, NULL, &stidx );
	stend = ftell( o );

	// dump sections
	jfuncoff = dump_section_jfuncs( o, jsection, jsize );
	// TODO: dump functions in own section

	// dump section header entries 
	sechdroff = ftell( o );
	dump_section( o, 0, SHT_NULL, 0, 0, 0, 0, SHN_UNDEF, 0 );				// null section
	dump_section( o, stridst, SHT_STRTAB, 0, 0, stoff, stend - stoff, SHN_UNDEF, 0 );	// string table
	// TODO: symbol table
	dump_section( o, stridtext, SHT_PROGBITS, SHF_ALLOC | SHF_EXECINSTR, (int)jsection, jfuncoff, jsize, 0, 0 );
	// TODO: text sections
	
	// goto start and dump header
	fseek( o, 0, SEEK_SET );
	dump_elf_header( o, 3, sechdroff, strtabidx );
	
	fclose( o );
}
