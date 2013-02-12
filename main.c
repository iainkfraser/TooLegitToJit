/*
* (C) Iain Fraser - GPLv3 
*/

#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <sys/time.h>
#include "lopcodes.h"
#include "mc_emitter.h"
#include "user_memory.h"

#define do_fail( msg, ... ) 	do{ fprintf( stderr, "error: " msg "\n", ##  __VA_ARGS__ ); goto exit; }while(0)
#define do_cfail( c, msg, ... )	do{ if( ( c ) ) do_fail( msg, ##  __VA_ARGS__ ); }while(0)


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

struct proto {
	int linedefined;
	int lastlinedefined;
	int sizecode;
	int nrconstants;	
	uint8_t numparams;
	uint8_t is_vararg;
	uint8_t maxstacksize;	
};

#define member_size(type, member) sizeof(((type *)0)->member)
#define do_load_member( type, member, ptr, f )	fread( &ptr -> member, member_size( type, member ), 1, f )
#define load_member( ptr, member, f )	do_load_member( typeof( *ptr ), member, ptr, f )


/*
* Function execution consists of:
* 	Function Prologue - load arguments and constants onto the stack
* 	Code - code loaded here  
* 	Function Epilogue - do return procedure
*	For now string them together using JMPs, in final version should just be one large allocation.
*
*	TODO:
*		+ determine where prologue will put args, locals and constants on the stack
*		+ what does dynasm return?
*
*	constants
*	--------
*	locals
*
*/
int load_code( FILE* f, struct proto* p ){
	uint32_t ins;
	int ret, seek;
	void* mce;

	load_member( p, sizecode, f );
	
	// skip over code to load constants
	seek = ftell( f );
	fseek( f, p->sizecode * 4, SEEK_CUR );
	load_member( p, nrconstants, f );
	
	mce_init( &mce, p->sizecode, p->nrconstants );
	
	// load constants and come back to code
	if( ret = load_constants( f, p, &mce ) )
		return ret;	
	fseek( f, seek, SEEK_SET );

	mce_start( &mce, p->maxstacksize );
	
	// jit the code
	for( unsigned int i = 0; i < p->sizecode; i++){
		fread( &ins, sizeof( uint32_t ), 1, f ); 

		uint32_t A = GETARG_A( ins );

		// emit Lua VM pc
		label( i, &mce );

// calculate position on stack of locals and constants.
#define KADDR( k ) ( 4 * ( p->maxstacksize + k ) ) 
#define REGADDR( x )	( ISK(x) ? KADDR( INDEXK( x ) ) : 4 * x )

		int reg1,reg2;

		switch( GET_OPCODE( ins ) ){
			case OP_LOADK:	// R(A) = Kst( Bx ) 
				emit_loadk( &mce, A,  GETARG_Bx( ins ) );
				break;
			case OP_ADD:
				emit_add( &mce, to_loperand( A ), 
					to_loperand( GETARG_B( ins ) ), 
					to_loperand( GETARG_C( ins ) ) );
				break;
			case OP_RETURN:
				emit_ret( &mce );
				break;
			case OP_MOVE:
				emit_move( &mce, to_loperand( A ), 
					to_loperand( GETARG_B( ins ) ) );
				break;
			case OP_FORPREP:
				emit_forprep( &mce, to_loperand( A ), i, 
						GETARG_sBx( ins ) );

				break;
			case OP_FORLOOP:
				emit_forloop( &mce, to_loperand( A ), i, 
						GETARG_sBx( ins ));
				break;
			case OP_NEWTABLE:
				emit_newtable( &mce, to_loperand( A ), 
						GETARG_B( ins ),
						GETARG_C( ins ) );
				break; 
			case OP_SETLIST:
				printf("SETLIST\n");
				break;
			default:
				printf("%d\n", GET_OPCODE( ins ) );
				assert( 0 );
		}

	}

	// link and copy the machine code
#if 0 
	size_t size = mce_link( &mce );
	char* mem = malloc( size );
	char* start = mce_stop( &mce, mem );
	printf("%x %x\n", mem, start );

	FILE* o = fopen("dsa","w");
	fwrite( mem, size, 1, o );
	fclose( o );
	free( mem );
#else
	size_t size = mce_link( &mce );
  	char *mem = em_alloc( size );
	
	int (*chunk)() = mce_stop( &mce, mem );
	em_execperm( mem, size );


	struct timeval tv;
	gettimeofday( &tv, NULL );
	uint32_t start = tv.tv_sec * 1000 + tv.tv_usec / 1000;
	int x = chunk();
//	asm("movl %%ecx, %0" : "=r" ( x ) ); 
	asm("move %0, $t1" : "=r" ( x ) );
	gettimeofday( &tv, NULL );
	uint32_t end = tv.tv_sec * 1000 + tv.tv_usec / 1000;
	printf("JIT %d took %u ms\n", x, end - start );

	em_free( mem, size );
#endif

	return 0;
}

/* load constants and generate function prologue */
int load_constants( FILE* f, struct proto* p, void** mce ){
	char t;
	int k; 
//	load_member( p, nrconstants, f );

	for( int i = 0; i < p->nrconstants; i++ ){
		fread( &t, sizeof(char), 1, f );	
		switch( t ){
			case LUA_TNUMBER:
				fread( &k, sizeof( int ), 1, f );	
		//		emit_setk( 4 * ( p->maxstacksize + i ), k );
				// TODO: use stack macro 
			//setk( 4 * ( p->maxstacksize + i ) , k, mce );
			
#if 1
				mce_const_int( mce, i, k );
#else
				loadim( 4 * ( p->maxstacksize + i ) , k, mce );
#endif
				break;	
			default:
				assert( 0 );
		}
	}	
	return 0;
}

/* load function prototype */ 
int load_function( FILE* f, struct proto* p ){
	int ret;

	assert( p );
	assert( f ) ;

	load_member( p, linedefined, f );
	load_member( p, lastlinedefined, f );
	load_member( p, numparams, f );
	load_member( p, is_vararg, f );
	load_member( p, maxstacksize, f );

	if( ret = load_code( f, p ) )
		return ret;

//	if( ret = load_constants( f, p ) )
//		return ret;	
}


int main( int argc, char* argv[] ){
	FILE* f = NULL;
	struct proto main;

	if( argc != 2 ) 
		do_fail("expected input filename");
	
	f = fopen( argv[1], "r" );
	if( !f )
		do_fail("unable to open file");

	// init jit
//	jit_init();

	do_cfail( validate_header( f ), "unacceptable header" );
	do_cfail( load_function( f, &main), "unable to load func" );

#if 0
	int (*chunk)() = jit_code();

	struct timeval tv;

	gettimeofday( &tv, NULL );
	uint32_t start = tv.tv_sec * 1000 + tv.tv_usec / 1000;

	int x = chunk();

	asm("movl %%ecx, %0" : "=r" ( x ) ); 

	gettimeofday( &tv, NULL );
	uint32_t end = tv.tv_sec * 1000 + tv.tv_usec / 1000;
	
	printf("JIT %d took %u ms\n", x, end - start );
	jit_freecode( chunk );
#endif

exit:
	if( f )	fclose( f );
	return 0;
}

