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

static void dump_elf_header( FILE* f ){
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
		.e_shoff = sizeof( Elf32_Ehdr ), 	// section header follows the elf header 
		.e_flags = 0,		// TODO: is this needed? 
		.e_ehsize = sizeof( Elf32_Ehdr ),
		.e_phentsize = sizeof( Elf32_Phdr ),
		.e_phnum = 0,
		.e_shentsize = sizeof( Elf32_Shdr ),
		.e_shnum = JF_COUNT + 2,	// null sec + strtab + symtab + every func + every jfunc	 
		.e_shtrndx = 1		// TODO: location of string table
	};


	fwrite( &hdr, sizeof( hdr ), 1, f );
}

static void dump_label( FILE* f, int n, int lbl[n], int *stidx ){
	for( int i = 0; i < n; i++ ){
		stidx += fprintf( f, "%d", lbl[i] );
		if( i + 1 < n )
			stidx += fputs( ".", f );
	}

	stidx += fputs( "\0", f );	
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

	for(int i = 0; i < p->nrprotos; i++ ){
		dump_stringtable_protos( f, &p->subp[i], i, n+1, label, stidx );
	}

	// TODO: rememmeber index into string table
	p->strtabidx = *stidx;
	dump_label( f, n + 1, label, stidx );	

}

static void dump_section_header( FILE* f ){
	Elf32_Shdr hdr[ JF_COUNT + 1 ] = {
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

void serialise( struct proto* main, char* filepath ){
	// TODO: recursively write instructions to file
	FILE* o = fopen( filepath, "w" );
	fwrite( main->code, main->sizemcode, 1, o );

	// TODO: dump header
	dump_elf_header( o );
	
	// dump string table
	int text, stidx = 1;
	stidx += fprintf( o, "\0" );
	text = stidx;
	stidx += fprintf( o, "text" );
	dump_stringtable_jfuncs( o, &stidx );
	dump_stringtable_protos( o, main, 0, 0, NULL, &stidx );

	// dump sections
	
	fclose( o );
}
