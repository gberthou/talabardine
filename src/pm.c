#include "pm.h"

struct __attribute__((packed)) pm_t
{
    uint8_t ctrl;
    uint8_t sleep;
    uint8_t _pad0[6];
    uint8_t cpusel;
    uint8_t apbsel[3];
    uint32_t _pad1[2];
    uint32_t ahbmask;
    uint32_t apbmask[3];
    uint32_t _pad2[4];
    uint8_t intenclr;
    uint8_t intenset;
    uint8_t intflag;
    uint8_t _pad3;
    uint8_t rcause;
};

#define PM ((volatile struct pm_t*) 0x40000400)

/*
 * Table 16-1. Peripheral Clock Default State
 * CLK_PAC0_APB    Enabled
 * CLK_PM_APB      Enabled
 * CLK_SYSCTRL_APB Enabled
 * CLK_GCLK_APB    Enabled
 * CLK_WDT_APB     Enabled
 * CLK_RTC_APB     Enabled
 * CLK_EIC_APB     Enabled
 * CLK_PAC1_APB    Enabled
 * CLK_DSU_APB     Enabled
 * CLK_NVMCTRL_APB Enabled
 * CLK_PORT_APB    Enabled
 * CLK_HMATRIX_APB Enabled
 * CLK_PAC2_APB    Disabled
 * CLK_SERCOMx_APB Disabled
 * CLK_TCx_APB     Disabled
 * CLK_ADC_APB     Enabled
 * CLK_ACx_APB     Disabled
 * CLK_DAC_APB     Disabled
 * CLK_PTC_APB     Disabled
 * CLK_USB_APB     Enabled
 * CLK_DMAC_APB    Enabled
 * CLK_TCCx_APB    Disabled
 * CLK_I2S_APB     Disabled
 */

void pm_enable_APB_clock(enum pm_clock_e clock, bool enable)
{
    volatile uint32_t *mask = &PM->apbmask[clock >> 5];
    size_t m = (1 << (clock & 0x1f));
    uint32_t tmp = *mask;
    if(enable)
        tmp |= m;
    else
        tmp &= ~m;
    *mask = tmp;
}

