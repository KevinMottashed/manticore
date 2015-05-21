/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <kevinmottashed@gmail.com> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return. 
 * -Kevin Mottashed
 * ----------------------------------------------------------------------------
 */

#include "pqueue.h"

#include "heap.h"

#include <assert.h>
#include <string.h>

#define MIN_PQUEUE_ALLOC (8)


void pqueue_init(pqueue_t * q)
{
  q->numElements = 0;
  q->elements = heap_malloc(MIN_PQUEUE_ALLOC * sizeof(q->elements[0]));
  assert(q->elements != NULL);
  q->sizeElements = MIN_PQUEUE_ALLOC;
}

bool pqueue_empty(const pqueue_t * q)
{
  return q->numElements == 0;
}

size_t pqueue_size(const pqueue_t * q)
{
  return q->numElements;
}

void pqueue_push(pqueue_t * q, void * data, int priority)
{
  // Allocate more memory if the queue is full.
  if (q->numElements == q->sizeElements)
  {
    // Grow the array by a factor of 2 when we run out of space.
    q->sizeElements *= 2;
    q->elements = heap_realloc(q->elements, q->sizeElements * sizeof(q->elements[0]));
    assert(q->elements != NULL);
  }
  
  // Find the position of the new element.
  int position;
  for (position = 0; position < q->numElements; ++position)
  {
    if (priority > q->elements[position].priority)
    {
      break;
    }
  }
  
  // Shift all the elements after the insertion point.
  // We don't need to do this if the element is being inserted at the end.
  if (position != q->numElements)
  {
    void * dest = &q->elements[position + 1];
    void * src = &q->elements[position];
    size_t len = (q->numElements - position) * sizeof(q->elements[0]);
    memmove(dest, src, len);
  }
  
  // Insert the new element.
  q->elements[position].data = data;
  q->elements[position].priority = priority;
  q->numElements++;
  return;  
}

void * pqueue_pop(pqueue_t * q)
{
  if (q->numElements == 0)
  {
    return NULL;
  }
  
  void * result = q->elements[0].data;
  
  q->numElements--;
  
  // Move the rest of the elements to the front of the queue.
  if (q->numElements != 0)
  {
    void * dest = q->elements;
    void * src = q->elements + 1;
    size_t len = q->numElements * sizeof(q->elements[0]);
    memmove(dest, src, len);
  }
  
  // TBD: Reallocate q->elements if it's mostly empty?
  return result;
}

void * pqueue_top(pqueue_t * q)
{
  if (q->numElements == 0)
  {
    return NULL;
  }
  return q->elements[0].data;
}
