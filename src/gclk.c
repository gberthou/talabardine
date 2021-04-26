#include <stdint.h>
#include "gclk.h"

// 15.1: Nine channels
//       Channel 0 = GCLK_MAIN
// 15.6.1: Only eight channels... eh
// page 134: GCLK ID goes from 0 to 8 -> 9 channels huh

struct __attribute__((packed)) gclk_t 
{
    uint8_t ctrl;
    uint8_t status;
    uint16_t clkctrl;
    uint32_t genctrl;
    uint32_t gendiv;
};

#define GCLK ((volatile struct gclk_t*) 0x40000c00)
#define NCHANS 9

static uint32_t frequencies_hz[NCHANS];

static unsigned int div_resolution(enum gclk_channel_e channel)
{
    if(channel == GCLK1)
        return 16;
    if(channel == GCLK2)
        return 5;
    return 8;
}

uint32_t gclk_set_frequency(enum gclk_channel_e channel, uint32_t frequency_hz)
{
    const uint32_t REF_FREQUENCY_HZ = 48000000;
    uint32_t divider = REF_FREQUENCY_HZ / frequency_hz;
    unsigned int resolution = div_resolution(channel);
    uint32_t maxdiv1 = (1u << resolution) - 1; // If DIVSEL == 0
    uint32_t maxdiv2 = (1u << (maxdiv1 + 1)); // If DIVSEL == 1
    uint32_t divsel = 0;
    uint32_t actual_frequency_hz;

    if(divider <= maxdiv1)
        actual_frequency_hz = REF_FREQUENCY_HZ / divider;
    else if(divider <= maxdiv2)
    {
        unsigned int i = 31;
        while(divider < (1u << i))
            --i;
        // Divider is now between (1<<i) and (1<<(i+1))
        if(i == 0)
            return 0xffffffff;
        divider = i - 1;
        if(divider > maxdiv1)
            return 0xffffffff;
        divsel = (1 << 20);
        actual_frequency_hz = (REF_FREQUENCY_HZ >> (divider + 1));
    }
    else
        return 0xffffffff;

    GCLK->gendiv = channel
                 | (divider << 8) // DIV
                 ; // Should be the same as reset state
    GCLK->genctrl = channel
                  | (0x07 << 8) // DFLL48M
                  | (1 << 16) // GENEN
                  | (1 << 17) // IDC
                  | (0 << 19) // OE (no gpio output)
                  | (0 << 21) // RUNSTDBY
                  | divsel
                  ;
    frequencies_hz[channel] = actual_frequency_hz;
    return actual_frequency_hz;
}

uint32_t gclk_get_frequency(enum gclk_channel_e channel)
{
    return frequencies_hz[channel];
}

void gclk_connect_clock(enum gclk_dst_e dst, uint8_t gclkid)
{
    gclkid %= NCHANS;

    GCLK->clkctrl = dst
                  | (gclkid << 8)
                  | (1 << 14) // CLKEN
                  ;
}

void gclk_disconnect_clock(enum gclk_dst_e dst)
{
    GCLK->clkctrl = dst
                  | (0 << 14) // CLKEN
                  ;
}

