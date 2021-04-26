#ifndef TC_H
#define TC_H

#include <stdint.h>
#include <stdbool.h>

#include "gclk.h"

enum tc_channel_e
{
    TC3 = 0,
    TC4,
    TC5,
    TC6,
    TC7
};

bool tc_init(enum tc_channel_e channel, enum gclk_channel_e gclk_src, uint32_t frequency_hz);
void tc_clear_interrupt(enum tc_channel_e channel);

#endif

