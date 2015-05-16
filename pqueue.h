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
 * This interface provides a priority queue similar to the one in the C++ STL.
 * The queue acts as a FIFO for elements of equal priority.
 * The underlying implementation is a vector.
 */

#ifndef PRIORITY_QUEUE_H
#define PRIORITY_QUEUE_H

#include <stddef.h>
#include <stdbool.h>

typedef struct pqueue_elem_s
{
  void * data;
  int priority;
} pqueue_elem_t;

typedef struct pqueue_s
{
  size_t numElements; // The number of elements in the priority queue
  pqueue_elem_t * elements; // Array of all the elements.
  size_t sizeElements; // The size of the elements array.
} pqueue_t;

void pqueue_init(pqueue_t * q);

bool pqueue_empty(const pqueue_t * q);
size_t pqueue_size(const pqueue_t * q);

void pqueue_push(pqueue_t * q, void * data, int priority);
void * pqueue_pop(pqueue_t * q);
void * pqueue_top(pqueue_t * q);

#endif
