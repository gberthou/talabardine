#ifndef ABP_SPI_H
#define ABP_SPI_H

#include <stdint.h>

#define ABP_MAX_PA         103421u // 15 psi
#define ABP_10_PERCENT_PA  10342u  // 10% max
#define ABP_90_PERCENT_PA  93078u  // 90% max
#define ABP_COUNT_MIN 0x0666u
#define ABP_COUNT_MAX 0x399au
#define ABP_PA_2_COUNTS(p) (ABP_COUNT_MIN + (int32_t)(((p) * ((float)(ABP_COUNT_MAX - ABP_COUNT_MIN) / (ABP_90_PERCENT_PA - ABP_10_PERCENT_PA)))))

uint16_t abp_get_pressure(void);
uint16_t abp_wait_until_valid_pressure(void);

#endif

