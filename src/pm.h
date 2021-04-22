#ifndef PM_H
#define PM_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

enum pm_clock_e
{
    PM_CLK_PAC0 = 0,
    PM_CLK_PM,
    PM_CLK_SYSCTRL,
    PM_CLK_GCLK,
    PM_CLK_WDT,
    PM_CLK_RTC,
    PM_CLK_EIC,

    PM_CLK_PAC1 = 32,
    PM_CLK_DSU,
    PM_CLK_NVMCTRL,
    PM_CLK_PORT,
    PM_CLK_DMAC,
    PM_CLK_USB,

    PM_CLK_PAC2 = 64,
    PM_CLK_EVSYS,
    PM_CLK_SERCOM0,
    PM_CLK_SERCOM1,
    PM_CLK_SERCOM2,
    PM_CLK_SERCOM3,
    PM_CLK_SERCOM4,
    PM_CLK_SERCOM5,
    PM_CLK_TCC0,
    PM_CLK_TCC1,
    PM_CLK_TCC2,
    PM_CLK_TC3,
    PM_CLK_TC4,
    PM_CLK_TC5,
    PM_CLK_TC6,
    PM_CLK_TC7,
    PM_CLK_ADC,
    PM_CLK_AC,
    PM_CLK_DAC,
    PM_CLK_PTC,
    PM_CLK_I2S,
    PM_CLK_AC1,

    PM_CLK_TCC3 = 88
};

void pm_enable_APB_clock(enum pm_clock_e clock, bool enable);

#endif

