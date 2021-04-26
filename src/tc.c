#include "tc.h"
#include "gclk.h"

struct __attribute__((packed)) tc8b_t
{
    uint16_t ctrla;
    uint16_t readreq;
    uint8_t ctrlbclr;
    uint8_t ctrlbset;
    uint8_t ctrlc;
    uint8_t _pad0;
    uint8_t dbgctrl;
    uint8_t _pad1;
    uint16_t evctrl;
    uint8_t intenclr;
    uint8_t intenset;
    uint8_t intflag;
    uint8_t status;
    uint8_t count;
    uint8_t _pad2[3];
    uint8_t per;
    uint8_t _pad3[3];
    uint8_t cc0;
    uint8_t cc1;
};

#define TC3_BASE 0x42002c00
#define TC_BASE(x) (TC3_BASE + ((x) << 10))
#define TC8BIT(x) ((volatile struct tc8b_t*) TC_BASE((x)))

#define STATUS_SYNCBUSY (1 << 7)

bool tc_init(enum tc_channel_e channel, enum gclk_channel_e gclk_src, uint32_t frequency_hz)
{
    volatile struct tc8b_t *tc8b = TC8BIT(channel);

    enum gclk_dst_e gclk;
    if(channel == TC3)
        gclk = GCLK_DST_TCC2_TC3;
    else if(channel <= TC5)
        gclk = GCLK_DST_TC45;
    else
        gclk = GCLK_DST_TC67;
    gclk_connect_clock(gclk, gclk_src); 

    uint32_t ref_frequency_hz = gclk_get_frequency(gclk_src);
    uint32_t divider = ref_frequency_hz / frequency_hz;
    uint32_t prescaler = 0;
    uint32_t shift = 0; // Extra shift for it seems that one PERiod = two runs
                        // (Figure 30-4)
    while(prescaler < 8)
    {
        uint32_t tmp = (divider >> shift);
        if(tmp <= 0xff)
        {
            divider = tmp;
            break;
        }
        shift += (prescaler < 4 ? 1 : 2);
        ++prescaler;
    }
    // Cannot encode the desired frequency on 8 bits wrt 48MHz reference
    if(divider > 255)
        return false;

    tc8b->ctrla = 0; // Disable
    while(tc8b->status & STATUS_SYNCBUSY);

    tc8b->ctrla = (0x1 << 2) // MODE 8bit mode
                | (0x0 << 5) // WAVEGEN Normal frequency
                | (prescaler << 8)
                | (0x1 << 12) // Use prescaler
                ;
    tc8b->ctrlbclr = (1 << 0) // Clear DIR -> incrementing counter
                   | (1 << 2) // Clear ONESHOT -> run continuously
                   ;
    while(tc8b->status & STATUS_SYNCBUSY);
    tc8b->ctrlc = 0;
    while(tc8b->status & STATUS_SYNCBUSY);
    tc8b->per = divider;
    while(tc8b->status & STATUS_SYNCBUSY);

    tc8b->intenclr = 0x3c;
    tc8b->intenset = (1 << 0); // Enable overflow interrupt
    tc8b->ctrla |= (1 << 1); // Enable

    return true;
}

void tc_clear_interrupt(enum tc_channel_e channel)
{
    volatile struct tc8b_t *tc8b = TC8BIT(channel);
    tc8b->intflag = (1 << 0); // Ack overflow interrupt
}

