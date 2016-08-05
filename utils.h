/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <kevinmottashed@gmail.com> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 * -Kevin Mottashed
 * ----------------------------------------------------------------------------
 */

#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>

#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))

// The chip only has 3 breakpoints so this is essential for debugging.
#define BREAKPOINT asm("BKPT #1")

#define TOKEN_PASTE_IMPL(x, y) x ## y
#define TOKEN_PASTE(x, y) TOKEN_PASTE_IMPL(x, y)

// Generates a unique variable name
#define UNIQUE_VAR(name) TOKEN_PASTE(__ ## name, __LINE__)

// Returns the offset of a given member in a type
#define offset_of(type, member)\
  (uint32_t)&(((type *)0)->member)

// Returns a pointer to the struct that the given member is contained in.
#define container_of(ptr, type, member)\
  ((type *)((char *)ptr - offset_of(type, member)))

#endif
