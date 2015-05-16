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
 * This interface provides a vector similar to the one in the C++ STL.
 */

#ifndef VECTOR_H
#define VECTOR_H

#include <stddef.h>
#include <stdbool.h>

typedef struct vector_s
{
  void ** array; // The elements in the vector
  size_t size;   // The size of the vector.
  size_t alloc;  // The number of elements allocated.
} vector_t;

// Initialization
void vector_init(vector_t * v);

// Capacity
size_t vector_size(vector_t * v);
bool vector_empty(vector_t * v);

// Insertion / Removal
void vector_push_back(vector_t * v, void * data);
void * vector_pop_back(vector_t * v);
void vector_erase(vector_t * v, size_t index);

// Access
void * vector_at(vector_t * v, size_t index);
void * vector_front(vector_t * v);
void * vector_back(vector_t * v);

#endif
