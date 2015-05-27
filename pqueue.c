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
#include <stdint.h>

#define MIN_PQUEUE_ALLOC (8)

// Sanity check.
// Should be used in assert() statements to make sure the queue is sane.
static bool pqueue_check(pqueue_t * q);

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
  assert(pqueue_check(q));
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
  
  assert(pqueue_check(q));
  
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
   
void * pqueue_at(pqueue_t * q, size_t position)
{
  assert(q != NULL);
  return q->elements[position].data;
}

void pqueue_reschedule(pqueue_t * q, void * data, int priority)
{
  assert(q != NULL);
  assert(data != NULL);
  
  int newPos = -1;
  int oldPos = -1;
  
  // We iterate through the elements until we find where the element
  // currently is and where it needs to go.
  for (int i = 0; i < q->numElements; ++i)
  {
    if (data == q->elements[i].data)
    {
      // We found the element to move.
      oldPos = i;
      
      if (priority == q->elements[i].priority)
      {
        // The old and new priorities are the same.
        return;
      }
      
      if (newPos != -1)
      {
        // Found both positions.
        break;
      }
      
      if ((i + 1 < q->numElements && priority >= q->elements[i+1].priority) ||
           i + 1 == q->numElements)
      {
        // The element is already in the right position because the next
        // element has a lower priority or the element is already the 
        // last in the queue.
        q->elements[i].priority = priority;
        assert(pqueue_check(q));
        return;
      }
    }
    
    if (priority > q->elements[i].priority && newPos == -1)
    {
      // We found the new position for this element.
      newPos = i;
      if (oldPos != -1)
      {
        // Found both positions.
        break;
      }
    }
  }
  
  if (newPos == -1)
  {
    newPos = q->numElements;
  }
  
  assert(oldPos != -1);
  assert(newPos != oldPos);
  
  // Shift all elements between old and new up/down by one position.
  // This will overwrite the old element and free a spot at the new position.
  if (oldPos > newPos)
  {
    memmove(&q->elements[newPos+1], 
            &q->elements[newPos], 
            (oldPos - newPos) * sizeof(q->elements[0]));
  }
  else // newPos > oldPos
  {
    newPos--;
    memmove(&q->elements[oldPos], 
            &q->elements[oldPos+1], 
            (newPos - oldPos) * sizeof(q->elements[0]));
  }
  q->elements[newPos].data = data;
  q->elements[newPos].priority = priority;
  assert(pqueue_check(q));
  return;
}

bool pqueue_check(pqueue_t * q)
{
  uint8_t lastPriority = 255;
  for (int i = 0; i < q->numElements; ++i)
  {
    if (q->elements[i].priority > lastPriority)
    {
      return false;
    }
    lastPriority = q->elements[i].priority;
  }
  return true;
}
