#ifndef GCLK_H
#define GCLK_H

#include <stdint.h>

enum gclk_dst_e
{
    GCLK_DST_DFLL48M_REF = 0,
    GCLK_DST_DPLL,
    GCLK_DST_DPLL_32K,
    GCLK_DST_WDT,
    GCLK_DST_RTC,
    GCLK_DST_EIC,
    GCLK_DST_USB,
    GCLK_DST_EVSYS_CHANNEL_0,
    GCLK_DST_EVSYS_CHANNEL_1,
    GCLK_DST_EVSYS_CHANNEL_2,
    GCLK_DST_EVSYS_CHANNEL_3,
    GCLK_DST_EVSYS_CHANNEL_4,
    GCLK_DST_EVSYS_CHANNEL_5,
    GCLK_DST_EVSYS_CHANNEL_6,
    GCLK_DST_EVSYS_CHANNEL_7,
    GCLK_DST_EVSYS_CHANNEL_8,
    GCLK_DST_EVSYS_CHANNEL_9,
    GCLK_DST_EVSYS_CHANNEL_10,
    GCLK_DST_EVSYS_CHANNEL_11,
    GCLK_DST_SERCOMx_SLOW,
    GCLK_DST_SERCOM0_CORE,
    GCLK_DST_SERCOM1_CORE,
    GCLK_DST_SERCOM2_CORE,
    GCLK_DST_SERCOM3_CORE,
    GCLK_DST_SERCOM4_CORE,
    GCLK_DST_SERCOM5_CORE,
    GCLK_DST_TCC01,
    GCLK_DST_TCC2_TC3,
    GCLK_DST_TC45,
    GCLK_DST_TC67,
    GCLK_DST_ADC,
    GCLK_DST_AC_DIG,
    GCLK_DST_AC_ANA,
    GCLK_DST_DAC,
    GCLK_DST_PTC,
    GCLK_DST_I2S_0,
    GCLK_DST_I2S_1
};

enum gclk_channel_e
{
    GCLK0 = 0,
    GCLK1,
    GCLK2,
    GCLK3,
    GCLK4,
    GCLK5,
    GCLK6,
    GCLK7,
    GCLK8
};

uint32_t gclk_set_frequency(enum gclk_channel_e channel, uint32_t frequency_hz);
/*
 * gclk_set_frequency sets the frequency of the given clock
 * Returns the actual clock frequency, in Hz, or 0xffffffff if that frequency is
 * not reachable from that particular channel.
 * Note that all channels do not have the same resolution (DIVSEL == 0)
 * 15.8.5:
 * GCLK1  16bit
 * GCLK2  5bit
 * others 8bit
 */

uint32_t gclk_get_frequency(enum gclk_channel_e channel);
/*
 * gclk_get_frequency returns the value returned by gclk_set_frequency for the
 * given clock (Hz)
 */

void gclk_connect_clock(enum gclk_dst_e dst, uint8_t gclkid);
void gclk_disconnect_clock(enum gclk_dst_e dst);

#endif

