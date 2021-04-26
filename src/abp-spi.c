#include "abp-spi.h"
#include "config.h"
#include "gpio.h"
#include "sercom.h"

uint16_t abp_get_pressure(void)
{
    uint16_t tx;
    uint16_t rx;
    sercom_spi_burst(SERCOM_PRESSURE_CHANNEL, GPIO_PORT_A, 6, &rx, &tx, sizeof(rx));
    return (rx << 8) | (rx >> 8);
}

uint16_t abp_wait_until_valid_pressure(void)
{
    for(;;)
    {
        uint16_t pressure = abp_get_pressure();
        if((pressure & 0xc0000) == 0) // Status bits should be 0
            return (pressure & 0xfff); // 12bit resolution
    }
}

