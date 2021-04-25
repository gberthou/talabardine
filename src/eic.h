#ifndef EIC_H
#define EIC_H

#include <stdint.h>

void eic_init(void);
void eic_enable(uint8_t id);
void eic_disable(uint8_t id);
void eic_clear(uint8_t id);

#endif

