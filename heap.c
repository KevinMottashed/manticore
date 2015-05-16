/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <kevinmottashed@gmail.com> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return. 
 * -Kevin Mottashed
 * ----------------------------------------------------------------------------
 */


#include "heap.h"

#include "system.h"

#include <stdlib.h>

void * heap_malloc(size_t size)
{
  // Thread safety is achieved by disabling interrupts.
  __disable_interrupt();
  void * ptr = malloc(size);
  __enable_interrupt();
  return ptr;
}

void heap_free(void * ptr)
{
  __disable_interrupt();
  free(ptr);
  __enable_interrupt();
}

void * heap_calloc(size_t num, size_t size)
{
  __disable_interrupt();
  void * ptr = calloc(num, size);
  __enable_interrupt();
  return ptr;
}

void * heap_realloc(void* ptr, size_t size)
{
  __disable_interrupt();
  void * ret = realloc(ptr, size);
  __enable_interrupt();
  return ret;
}
