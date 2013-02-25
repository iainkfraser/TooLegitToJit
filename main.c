/*
* (C) Iain Fraser - GPLv3 
*
* TODO: This code is pretty much like a prototype so there are some serious problems:
*	1) Doesn't do any error checking on file read
*	2) File access interface should be decoupled from libc 
*/

#include <stdbool.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include "emitter.h"
#include "machine.h"
#include "lopcodes.h"
#include "frame.h"
#include "instruction.h"
#include "user_memory.h"
#include "list.h"
#include "math_util.h"



struct code_alloc {
	void* ( *alloc )( size_t );
	void  ( *free )( void*, size_t );
	void  ( *execperm )( void*, size_t ); 
};;

#define member_size(type, member) sizeof(((type *)0)->member)
#define do_load_member( type, member, ptr, f )	fread( &ptr -> member, member_size( type, member ), 1, f )
#define load_member( ptr, member, f )	do_load_member( typeof( *ptr ), member, ptr, f )

#define do_fail( msg, ... ) 	do{ fprintf( stderr, "error: " msg "\n", ##  __VA_ARGS__ ); goto exit; }while(0)
#define do_cfail( c, msg, ... )	do{ if( ( c ) ) do_fail( msg, ##  __VA_ARGS__ ); }while(0)

int load_function( FILE* f, struct proto* p, struct code_alloc* ca, struct machine* m, struct machine_ops* mops );


/* mark for precompiled code ('<esc>Lua') */
#define LUA_SIGNATURE	"\033Lua"

/* data to catch conversion errors */
#define LUAC_TAIL		"\x19\x93\r\n\x1a\n"

/* size in bytes of header of binary files */
#define LUAC_HEADERSIZE		(sizeof(LUA_SIGNATURE)-sizeof(char)+2+6+sizeof(LUAC_TAIL)-sizeof(char))

/* return strlen of constant string execluding NULL */
#define STRSIZEOF( x ) 	( sizeof( x ) - sizeof( char ) ) 


int validate_header( FILE* f ){
	char header[ LUAC_HEADERSIZE ]; 
	// write fread macro that fails
	fread( header, LUAC_HEADERSIZE, 1, f );

	if( !memcmp( LUA_SIGNATURE, header, STRSIZEOF( LUA_SIGNATURE )  ) ){
		char* seek = header + STRSIZEOF( LUA_SIGNATURE );
		if(
			*seek++ == 0x52			&&	// version 5.2
			*seek++ == 0x00 		&&	// official version
			*seek++ == 0x01			&&	// little endian, TODO: make it endianess of this comp
			*seek++ == sizeof(int)		&&	
			*seek++ == sizeof(size_t)	&&
			*seek++ == 0x04			&&	// size of instruction
			*seek++ == sizeof(int)		&& 	// sizeof( number ) we always use int
			*seek++ == 1			) {	// 1 is integral type i.e. not floating point ( 0 )	 
				
			return memcmp( seek, LUAC_TAIL, STRSIZEOF( LUAC_TAIL ) );
		}
	}

	return 1;	// failed 	
}

;

/* load prototypes */
int load_prototypes( FILE* f, struct proto* p, struct emitter** mce, struct code_alloc* ca, 
		struct frame* fr, struct machine_ops* mop ){
	int ret;
	load_member( p, nrprotos, f );
//	mce_proto_init( mce, p->nrprotos );	


	p->subp = malloc( sizeof( struct proto ) * p->nrprotos );
	if( !p->subp )
		return -ENOMEM;		// TODO: free memory for other children 

	for( int i = 0; i < p->nrprotos; i++ ){
		printf("loading prototype %d\n", i );

		struct proto* child = &p->subp[i];

		if( ret = load_function( f, child, ca, fr->m, mop ) )
			return ret;		// TODO: free memory 

		printf("loaded sub prototype %d\n", i );
	//	mce_proto_set( mce, i, child->code );
	}

	return 0;
}

/* load constants and generate function prologue */
int load_constants( FILE* f, struct proto* p, struct emitter** mce, struct frame* fr ){
	char t;
	int k; 

	load_member( p, nrconstants, f );
	init_consts( fr, p->nrconstants );

	for( int i = 0; i < p->nrconstants; i++ ){
		fread( &t, sizeof(char), 1, f );	
		switch( t ){
			case LUA_TNUMBER:
				fread( &k, sizeof( int ), 1, f );	
				setk_number( fr, i, k );
				break;	
			default:
				assert( 0 );
		}
	}	
	return 0;
}

void load_static_string( FILE* f, char* buffer, int len ){
	size_t size;
	fread( &size, sizeof( size_t ), 1, f );

	size_t read = min( len, size );
	size_t skip = max( (int)(size - len), 0 );

	fread( buffer, 1, read, f );
	fseek( f, skip, SEEK_CUR );

}

int ignore_debug( FILE* f, struct proto* p ){
	int nr_lineinfo, nr_localvars, nr_upvalues, i;
	char buffer[32];	// use to verify that loader is working

	// load source file
	load_static_string( f, buffer, sizeof( buffer ) );

	// load line info 
	fread( &nr_lineinfo, sizeof( int ), 1, f );
	fseek( f, nr_lineinfo * sizeof( int ), SEEK_CUR );

	// load local variable info
	fread( &nr_localvars, sizeof( int ), 1, f );
	for( i = 0; i < nr_localvars; i++ ){
		load_static_string( f, buffer, sizeof(buffer) );	// variable name
		fseek( f, 2 * sizeof( int ), SEEK_CUR );	// start and end pc 
	}

	// load upvalues names
	fread( &nr_upvalues, sizeof( int ), 1, f );
	for( i = 0; i < nr_upvalues; i++ )
		load_static_string( f, buffer, sizeof( buffer ) );

	return 0;
}

int ignore_upvalues( FILE* f, struct proto* p ){
	int n; char up[2];
	fread( &n, sizeof( int ), 1, f );
	for( int i = 0; i < n; i++ ){
		fread( up, 1, 2, f );
	}

	return 0; 
}


int load_code( FILE* f, struct proto* p, struct code_alloc* ca, struct machine* m, struct machine_ops* mop ){
	uint32_t ins;
	int ret, seek, end;
	struct emitter* mce;
	struct frame fr = { .m = m, .nr_locals = p->maxstacksize, .nr_params = p->numparams };

	// prepare function machine code emitter
	load_member( p, sizecode, f );
	mop->create_emitter( &mce, p->sizecode );

	// Skip over code. Rewind after constants loaded 
	seek = ftell( f );
	fseek( f, p->sizecode * 4, SEEK_CUR );

	// need the # of constants to create the frame 

	// load constants and come back to code
	if( ret = load_constants( f, p, &mce, &fr ) )
		return ret;	
	
	// load prototypes
	if( ret = load_prototypes( f, p, &mce, ca, &fr, mop ) )
		return ret;
	
	// rewind
	end = ftell( f );
	fseek( f, seek, SEEK_SET );

	// start machine code generation
	emit_header( mce, &fr );
//	mce_start( &mce, p->maxstacksize, p->numparams  );
	
	// jit the code
	for( unsigned int i = 0; i < p->sizecode; i++){
		fread( &ins, sizeof( uint32_t ), 1, f ); 

		uint32_t A = GETARG_A( ins );

		// emit Lua VM pc
		mce->ops->label_pc( mce, i );

// calculate position on stack of locals and constants.
#define KADDR( k ) ( 4 * ( p->maxstacksize + k ) ) 
#define REGADDR( x )	( ISK(x) ? KADDR( INDEXK( x ) ) : 4 * x )

		int reg1,reg2;

		switch( GET_OPCODE( ins ) ){
			case OP_LOADK:	// R(A) = Kst( Bx ) 
				emit_loadk( &mce, mop, &fr, A,  GETARG_Bx( ins ) );
				break;
			case OP_ADD:
				emit_add( &mce, mop, &fr, to_loperand( A ), 
					to_loperand( GETARG_B( ins ) ), 
					to_loperand( GETARG_C( ins ) ) );
				break;
			case OP_SUB:
				emit_sub( &mce, mop, &fr, to_loperand( A ), 
					to_loperand( GETARG_B( ins ) ), 
					to_loperand( GETARG_C( ins ) ) );
				break;
			case OP_DIV:
				emit_div( &mce, mop, &fr, to_loperand( A ), 
					to_loperand( GETARG_B( ins ) ), 
					to_loperand( GETARG_C( ins ) ) );
				break;
			case OP_MUL:
				emit_mul( &mce, mop, &fr, to_loperand( A ), 
					to_loperand( GETARG_B( ins ) ), 
					to_loperand( GETARG_C( ins ) ) );
				break;
			case OP_MOD:
				emit_mod( &mce, mop, &fr, to_loperand( A ), 
					to_loperand( GETARG_B( ins ) ), 
					to_loperand( GETARG_C( ins ) ) );
				break;
			case OP_RETURN:
				emit_ret( &mce, mop, &fr );
				break;
			case OP_MOVE:
				emit_move( &mce, mop, &fr, to_loperand( A ), 
					to_loperand( GETARG_B( ins ) ) );
				break;
			case OP_FORPREP:
				emit_forprep( &mce, mop, &fr, to_loperand( A ), i, 
						GETARG_sBx( ins ) );

				break;
			case OP_FORLOOP:
				emit_forloop( &mce, mop, &fr, to_loperand( A ), i, 
						GETARG_sBx( ins ));
				break;
			case OP_NEWTABLE:
				emit_newtable( &mce, mop, &fr, to_loperand( A ), 
						GETARG_B( ins ),
						GETARG_C( ins ) );
				break; 
			case OP_SETLIST:
				emit_setlist( &mce, mop, &fr, to_loperand( A ),
						GETARG_B( ins ),
						GETARG_C( ins ) );
				break;
			case OP_GETTABLE:
				emit_gettable( &mce, mop, &fr, to_loperand( A ),
					to_loperand( GETARG_B( ins ) ), 
					to_loperand( GETARG_C( ins ) ) );
				break;
			case OP_LOADNIL:
				// TODO: pretty simple	
				break;
			case OP_CLOSURE:{
				int idx = GETARG_Bx( ins );
				assert( idx < p->nrprotos );	// TODO: integrate into error code
 			
				// TODO: deal with reference counting of subprototype 	
				emit_closure( &mce, mop, &fr, to_loperand( A ), &p->subp[ idx ] );
			
				}break;
			case OP_CALL:
				emit_call( &mce, mop, &fr, to_loperand( A ), GETARG_B( ins ), 
						GETARG_C( ins )	);
				break;
			default:
				printf("%d\n", GET_OPCODE( ins ) );
				assert( 0 );
		}

	}

	emit_footer( mce, &fr );
	
	p->sizemcode = mce->ops->link( mce );
	p->code = ca->alloc( p->sizemcode );
	if( !p->code )
		assert( 0 );	

	p->code_start = mce->ops->stop( mce, p->code, 0 );
	if( ca->execperm )
		ca->execperm( p->code, p->sizemcode );
	

	fseek( f, end, SEEK_SET );
	return 0;
}

/* load function prototype */ 
int load_function( FILE* f, struct proto* p, struct code_alloc* ca, struct machine* m, struct machine_ops* mop ){
	int ret;

	assert( p );
	assert( f ) ;

	load_member( p, linedefined, f );
	load_member( p, lastlinedefined, f );
	load_member( p, numparams, f );
	load_member( p, is_vararg, f );
	load_member( p, maxstacksize, f );

	if( ret = load_code( f, p, ca, m, mop ) )
		return ret;

	ignore_upvalues( f, p );
	ignore_debug( f, p );
	

	return 0;

}

static void execute( struct proto* main ){
	struct timeval tv;
	int x;
	uint32_t start,end;
	int (*chunk)() = main->code_start;

	gettimeofday( &tv, NULL );
	start = tv.tv_sec * 1000 + tv.tv_usec / 1000;
	chunk();

	// temp method for getting result
#if defined(__mips__)
	asm("move %0, $t2" : "=r" ( x ) );
#elif defined( __i386__ )
	asm("movl %%ecx, %0" : "=r" ( x ) ); 
#endif

	gettimeofday( &tv, NULL );
	end = tv.tv_sec * 1000 + tv.tv_usec / 1000;


	printf("2 Legit 2 JIT %d took %u ms\n", x, end - start );
}

static void serialise( struct proto* main, char* filepath ){
	// TODO: recursively write instructions to file
	FILE* o = fopen( filepath, "w" );
	fwrite( main->code, main->sizemcode, 1, o );
	fclose( o );
}

static void cleanup( struct proto* p, struct code_alloc* ca ){
	for( int i = 0; i < p->nrprotos; i++ ){
		cleanup( &p->subp[i], ca );
	}

	// even safe for 0 child because subp is always malloc. If confused see man.
	ca->free( p->code, p->sizemcode );
	free( p->subp );
}

int main( int argc, char* argv[] ){
	FILE* f = NULL;
	struct proto main;
	bool disassem = false;
	char* out;
	int c;
	
	while( ( c = getopt( argc, argv, "d:" ) ) != -1 ){
		switch( c ){
			case 'd':
				disassem = true;
				out = optarg;
				break;
			case '?':
			default:
				do_fail("unknown option");
		}
	}
	
	if( optind >= argc )
		do_fail("expected input filename");
	
	f = fopen( argv[optind], "r" );
	if( !f )
		do_fail("unable to open file");

	struct code_alloc ca = {
		.alloc = &em_alloc,
		.free = &em_free,
		.execperm = &em_execperm
	};


	extern struct machine mips_mach;
	extern struct machine_ops mips_ops;

	// init jit
	do_cfail( validate_header( f ), "unacceptable header" );
	do_cfail( load_function( f, &main, &ca, &mips_mach, &mips_ops ), "unable to load func" );

	if( !disassem )
		execute( &main );
	else
		serialise( &main, out );	

	cleanup( &main, &ca );
exit:
	if( f )	fclose( f );
	return 0;
}

