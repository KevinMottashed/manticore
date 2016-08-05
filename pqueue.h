/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <kevinmottashed@gmail.com> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 * -Kevin Mottashed
 * ----------------------------------------------------------------------------
 */
#ifndef PQUEUE_H
#define PQUEUE_H

// This priority queue implementation is just a wrapper around
// a regular doubly-linked list.
#include "list.h"

// Returns true if a has a higher priority than b.
typedef bool (*pqueue_compare)(struct list_head * a, struct list_head * b);

struct pqueue
{
  pqueue_compare compare;
  struct list_head list;
};

void pqueue_init(struct pqueue * pqueue, pqueue_compare compare);
bool pqueue_empty(struct pqueue * pqueue);
uint32_t pqueue_size(struct pqueue * pqueue);
struct list_head * pqueue_peek(struct pqueue * pqueue);

struct list_head * pqueue_pop(struct pqueue * pqueue);
void pqueue_push(struct pqueue * pqueue, struct list_head * elem);

// TODO This needs to be called from task_reschedule.
// Reschedules an element after its priority has changed.
void pqueue_increase(struct pqueue * pqueue, struct list_head * elem);
void pqueue_decrease(struct pqueue * pqueue, struct list_head * elem);

#define pqueue_for_each(it, pqueue) list_for_each((it), &(pqueue)->list)

#endif
