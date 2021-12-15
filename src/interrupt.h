#ifndef INTERRUPT_H
#define INTERRUPT_H

#define interrupt_enable() __asm__ __volatile__("cpsie i")
#define interrupt_disable() __asm__ __volatile__("cpsid i")

#endif

