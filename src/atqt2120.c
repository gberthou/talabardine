#include "sercom.h"

#define SERCOMID 4
#define ATQT2120_I2C_ADDRESS 0x1c

void atqt2120_test(void)
{
    uint8_t address = 0;
    uint8_t ret[16];
    for(size_t i = 0; i < sizeof(ret); ++i)
        ret[i] = 0xde;

    sercom_i2c_write(SERCOMID, ATQT2120_I2C_ADDRESS, &address, 1);
    sercom_i2c_read(SERCOMID, ATQT2120_I2C_ADDRESS, ret, sizeof(ret));

    for(size_t i = 0; i < sizeof(ret); ++i)
        sercom_usart_display_half(ret[i]);
}


/*
void atqt2120_write(uint8_t address, uint8_t data)
{
}
*/

