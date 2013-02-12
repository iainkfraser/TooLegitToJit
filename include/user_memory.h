/*
* (C) Iain Fraser - GPLv3 
*/

#include <sys/mman.h>

static inline void* em_alloc( size_t size ){
	void *mem = mmap(NULL, size + sizeof(size_t), PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, 0, 0);
	assert(mem != MAP_FAILED);
	return mem;
}


static inline void em_execperm( void* mem, size_t size ){
  	int success = mprotect(mem, size, PROT_EXEC | PROT_READ);
  	assert(success == 0);
}

static inline void em_free( void* mem, size_t size ){
	int status = munmap( mem, size);
}

