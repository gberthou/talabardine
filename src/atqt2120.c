#include "atqt2120.h"
#include "sercom.h"

#define SERCOMID 4
#define ATQT2120_I2C_ADDRESS 0x1c

#define NKEYS 12

#define ADD_DETECTION_STATUS 2
#define ADD_RESET 7
#define ADD_KEYCTRL0 28

/* const uint8_t *data format:
 *    data[0] = local address
 *    data[1...] = data0...
 * size_t length == length of data
 */
#define i2c_write(data, length) sercom_i2c_write(SERCOMID, ATQT2120_I2C_ADDRESS, (data), (length))

/* uint8_t address == local address
 * uint8_t *data format:
 *    data[0...] = data0...
 * size_t length == length of data
 */
#define i2c_read(address, data, length) do{\
    uint8_t _address = (address);\
    i2c_write(&_address, 1);\
    sercom_i2c_read(SERCOMID, ATQT2120_I2C_ADDRESS, (data), (length));\
} while(0)

#if 0
void atqt2120_test(void)
{
    uint8_t ret[16];
    for(size_t i = 0; i < sizeof(ret); ++i)
        ret[i] = 0xde;

    i2c_read(0, ret, sizeof(ret));

    for(size_t i = 0; i < sizeof(ret); ++i)
        sercom_usart_display_half(ret[i]);
}
#endif

void atqt2120_init(const struct atqt2120_t *config)
{
    uint8_t tmp[NKEYS + 1];
    // 1. RESET
    tmp[0] = ADD_RESET;
    tmp[1] = 1;
    i2c_write(tmp, 2);
    while(!gpio_read(config->change_port, config->change_pin));

    // 2. Configure keys
    tmp[0] = ADD_KEYCTRL0;
    for(size_t i = 0; i < NKEYS;)
    {
        union {
            struct atqt2120_key_t key;
            uint8_t data;
        } k = {.key = config->keys[i]};
        tmp[++i] = k.data;
    }
    i2c_write(tmp, NKEYS + 1);
}

uint8_t atqt2120_read_status(void)
{
#if 0
    uint8_t ret;
    i2c_read(ADD_STATUS, &ret, 1);
    return ret;
#else
    uint8_t tmp[3];
    i2c_read(ADD_DETECTION_STATUS, tmp, 3);
    return tmp[2];
#endif
}

