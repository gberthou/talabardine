#include <stdint.h>
#include "gclk.h"
#include "nvmctrl.h"

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

void gclk_main_48MHz(void)
{
    // 37.12: NVM needs at least 1 wait state @48MHz when Vcc > 2.7v
    nvmctrl_set_wait_states(1);

    // GCLK_MAIN == GCLK0
    // GCLK0 has8 bit divider
    GCLK->gendiv = 0x00 // GCLK0
                 | (0x00 << 8) // DIV
                 ; // Should be the same as reset state
    GCLK->genctrl = 0x00 // GCLK0
                  | (0x07 << 8) // DFLL48M
                  | (1 << 16) // GENEN
                  | (1 << 17) // IDC
                  | (0 << 19) // OE (no gpio output)
                  | (0 << 20) // Divide by gendiv.DIV
                  | (0 << 21) // RUNSTDBY
                  ;
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

