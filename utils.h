#ifndef UTILS_H
#define UTILS_H

#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))

// The chip only has 3 breakpoints so this is essential for debugging.
#define BREAKPOINT asm("BKPT #1")

#endif