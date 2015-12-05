/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <kevinmottashed@gmail.com> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 * -Kevin Mottashed
 * ----------------------------------------------------------------------------
 */
#ifndef LIST_H
#define LIST_H

#include "util.h"

struct list_head
{
  struct list_head * next;
  struct list_head * prev;
};

// Initialize a list
void list_init(struct list_head * list);

// Add a node to the front/back of the list
void list_push_front(struct list_head * list, struct list_head * node);
void list_push_back(struct list_head * list, struct list_head * node);

// Remove an element from the list.
void list_remove(struct list_head * node);

// Iterates over a list. It's not safe to remove items from the list
// with this version. Use list_for_each_safe() instead.
#define list_for_each(it, list)\
  for (assert((list) != NULL),\
       assert((list)->next != NULL),\
       it = (list)->next;\
       it != (list);\
       assert(it->next != NULL),\
       it = it->next)

// Iterates over a list. It's safe to remove items from the list while iterating.
#define list_for_each_safe(it, list)\
  struct list_head * UNIQUE_VAR(next);\
  for (assert((list) != NULL),\
       assert((list)->next != NULL),\
       assert((list)->next->next != NULL),\
       it = (list)->next,\
       UNIQUE_VAR(next) = it->next;\
       it != list;\
       assert(UNIQUE_VAR(next)->next != NULL),\
       it = UNIQUE_VAR(next),\
       UNIQUE_VAR(next) = UNIQUE_VAR(next)->next)

#endif
