/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <kevinmottashed@gmail.com> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return. 
 * -Kevin Mottashed
 * ----------------------------------------------------------------------------
 */

/* 
 * This heap implementation simply wraps the standerd
 * memory functions for thread safety.
 */

#ifndef HEAP_H
#define HEAP_H

#include <stddef.h>

void * heap_malloc(size_t size);
void heap_free(void * ptr);
void * heap_calloc(size_t num, size_t size);
void * heap_realloc (void* ptr, size_t size);

#endif
