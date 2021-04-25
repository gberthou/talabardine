#include "eic.h"
#include "gclk.h"

struct __attribute__((packed)) eic_t
{
    uint8_t ctrl;
    uint8_t status;
    uint8_t nmictrl;
    uint8_t nmiflag;
    uint32_t evctrl;
    uint32_t intenclr;
    uint32_t intenset;
    uint32_t intflag;
    uint32_t wakeup;
    uint32_t config;
};

#define EIC ((volatile struct eic_t*) 0x40001800)

void eic_init(void)
{
    // CLK_EIC_APB enabled on reset
    gclk_connect_clock(GCLK_DST_EIC, 0); 

    EIC->config = (0x2 << 20); // Falling edge
                       // No filter

    EIC->ctrl = (1 << 1); // ENABLE
    while(EIC->status & (1 << 7)); // SYNCBUSY
    EIC->intflag = 0x3ffff; // Clear all former interrupt flags, if any

    // EIC.EVCTRL
    // EIC.WAKEUP
    // EIC.CONFIG
    // CTRL.ENABLE = 1
}

void eic_enable(uint8_t id)
{
    id %= 18;
    EIC->intenset = (1u << id);
}

void eic_disable(uint8_t id)
{
    id %= 18;
    EIC->intenclr = (1u << id);
}

void eic_clear(uint8_t id)
{
    EIC->intflag = (1u << id);
}

