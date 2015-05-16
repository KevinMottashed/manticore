/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <kevinmottashed@gmail.com> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return. 
 * -Kevin Mottashed
 * ----------------------------------------------------------------------------
 */

#include "vector.h"

#include "heap.h"

#include <assert.h>
#include <string.h>

#define MIN_VECTOR_ALLOC (8)

void vector_init(vector_t * v)
{
  v->size = 0;
  v->alloc = MIN_VECTOR_ALLOC;
  v->array = heap_malloc(MIN_VECTOR_ALLOC * sizeof(v->array));
  assert(v->array != NULL);
}

size_t vector_size(vector_t * v)
{
  assert(v != NULL);
  return v->size;
}

bool vector_empty(vector_t * v)
{
  assert(v != NULL);
  return v->size != 0;
}

void vector_push_back(vector_t * v, void * data)
{
  assert(v != NULL);
  if (v->size == v->alloc)
  {
    // Grow the vector by a factor of 2 when we run out of space.
    v->alloc *= 2;
    v->array = heap_realloc(v->array, v->alloc * sizeof(v->array[0]));
    assert(v->array != NULL);
  }
  
  v->array[v->size++] = data;
  return;
}

void * vector_pop_back(vector_t * v)
{
  assert(v != NULL);
  v->size--;
  // TBD Shrink the array if it's mostly empty?
  return v->array[v->size];
}

void vector_erase(vector_t * v, size_t index)
{
  assert(v != NULL);
  assert(index < v->size);
  v->size--;
  memmove(&v->array[index], 
          &v->array[index + 1], 
          (v->size - index) * sizeof(v->array[0]));
  
  // TBD Shrink the array if it's mostly empty?
  return;
}

void * vector_at(vector_t * v, size_t index)
{
  assert(v != NULL);
  assert(index < v->size);
  return v->array[index];
}

void * vector_front(vector_t * v)
{
  assert(v != NULL);
  return v->array[0];
}

void * vector_back(vector_t * v)
{
  assert(v != NULL);
  return v->array[v->size - 1];  
}

