#ifndef HARDWARE_H
#define HARDWARE_H

// The 8MHz HSE clock will be multiplied by 4 and used as the system clock.
#define SYSTEM_CLOCK (32000000)

void hardware_init(void);

#endif
