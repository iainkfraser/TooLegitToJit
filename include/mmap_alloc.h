/*
* (C) Iain Fraser - GPLv3 
*/

#ifndef _MMAP_ALLOC_H_
#define _MMAP_ALLOC_H_


void* mmap_alloc( size_t size );
void mmap_execperm( void* mem, size_t size );
void mmap_free( void* mem, size_t size );
void* mmap_realloc( void *ptr, size_t old_size, size_t new_size );

#endif
