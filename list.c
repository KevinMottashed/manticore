/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <kevinmottashed@gmail.com> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 * -Kevin Mottashed
 * ----------------------------------------------------------------------------
 */

#include "list.h"

#include <stddef.h>
#include <assert.h>

void list_init(struct list_head * list)
{
  list->next = list;
  list->prev = list;
}

void list_push_front(struct list_head * head, struct list_head * node)
{
  assert(head != NULL);
  assert(head->next != NULL);
  assert(head->prev != NULL);
  assert(node != NULL);

  node->next = head->next;
  node->prev = head;
  head->next->prev = node;
  head->next = node;
}

void list_push_back(struct list_head * head, struct list_head * node)
{
  assert(head != NULL);
  assert(head->next != NULL);
  assert(head->prev != NULL);
  assert(node != NULL);

  node->next = head;
  node->prev = head->prev;
  head->prev->next = node;
  head->prev = node;
}

void list_remove(struct list_head * node)
{
  assert(node != NULL);
  assert(node->next != NULL);
  assert(node->prev != NULL);

  node->prev->next = node->next;
  node->next->prev = node->prev;
  node->next = node;
  node->prev = node;
}
