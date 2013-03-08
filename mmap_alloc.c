/*
* (C) Iain Fraser - GPLv3 
*/

#ifdef __linux__
#define _GNU_SOURCE  
#endif

#include <assert.h>
#include <sys/mman.h>
#include <stddef.h>
#include "mmap_alloc.h"

void* mmap_alloc( size_t size ){
	void *mem = mmap(NULL, size + sizeof(size_t), PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, 0, 0);
	assert(mem != MAP_FAILED);			// TODO: error handling 
	return mem;
}

void* mmap_realloc( void *ptr, size_t old_size, size_t new_size ){
	void* mem;
	
	if( ptr == NULL )
		return mmap_alloc( new_size );

#ifdef __linux__
	mem = mremap( ptr, old_size, new_size, 0 );  
	assert( mem != MAP_FAILED );			// TODO: error handling 
#else
	// TODO: munmap the mmap  
#endif		
	return mem;
}

void mmap_execperm( void* mem, size_t size ){
  	int success = mprotect(mem, size, PROT_EXEC | PROT_READ);
  	assert(success == 0);
}

void mmap_free( void* mem, size_t size ){
	int status = munmap( mem, size);
}
