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

#include <assert.h>

void pqueue_init(struct pqueue * pqueue, pqueue_compare compare)
{
  assert(pqueue);
  assert(compare);
  pqueue->compare = compare;
  list_init(&pqueue->list);
}

bool pqueue_empty(struct pqueue * pqueue)
{
  assert(pqueue);
  return list_empty(&pqueue->list);
}

uint32_t pqueue_size(struct pqueue * pqueue)
{
  assert(pqueue);
  return list_size(&pqueue->list);
}

struct list_head * pqueue_peek(struct pqueue * pqueue)
{
  assert(pqueue);
  return list_front(&pqueue->list);
}

struct list_head * pqueue_pop(struct pqueue * pqueue)
{
  assert(pqueue);
  struct list_head * result = list_front(&pqueue->list);
  list_remove(result);
  return result;
}

void pqueue_push(struct pqueue * pqueue, struct list_head * elem)
{
  assert(pqueue);
  assert(elem);
  struct list_head * it;
  list_for_each_reverse(it, &pqueue->list) {
    if (!pqueue->compare(elem, it))
      break;
  }
  list_insert(it, elem);
}

void pqueue_increase(struct pqueue * pqueue, struct list_head * elem)
{
  assert(pqueue);
  assert(elem);
  struct list_head * it;
  for (it = elem->prev; it != &pqueue->list; it = it->prev) {
    if (!pqueue->compare(elem, it))
      break;
  }
  if (it->next != elem) {
    list_remove(elem);
    list_insert(it, elem);
  }
}

void pqueue_decrease(struct pqueue * pqueue, struct list_head * elem)
{
  assert(pqueue);
  assert(elem);
  struct list_head * it;
  for (it = elem; it->next != &pqueue->list; it = it->next) {
    if (!pqueue->compare(it->next, elem))
      break;
  }
  if (it != elem) {
    list_remove(elem);
    list_insert(it, elem);
  }
}
