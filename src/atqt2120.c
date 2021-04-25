#include "atqt2120.h"
#include "sercom.h"

#define SERCOMID 4
#define ATQT2120_I2C_ADDRESS 0x1c

#define NKEYS 12

#define ADD_DETECTION_STATUS 2
#define ADD_RESET 7
#define ADD_DI 11
#define ADD_DTHR0 16

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
    union key_u {
        struct atqt2120_key_t key;
        uint8_t data[2];
    };

    uint8_t tmp[2 * NKEYS + 1];
    
    // Temporarily set the /CHANGE pin as polled input
    gpio_configure_io(config->change_port, config->change_pin, false);

    // 1. RESET
    tmp[0] = ADD_RESET;
    tmp[1] = 1;
    i2c_write(tmp, 2);

    while(gpio_read(config->change_port, config->change_pin));
    atqt2120_read_status();
    while(!gpio_read(config->change_port, config->change_pin));

    tmp[0] = 16; // DTHR
    for(size_t i = 0; i++ < NKEYS;)
        tmp[i] = 64;
    i2c_write(tmp, NKEYS + 1);
    
    // 2. Set DI (lower = higher time-related reactivity)
    tmp[0] = ADD_DI;
    tmp[1] = config->di;
    i2c_write(tmp, 2);

    // 3. Configure keys
    tmp[0] = ADD_DTHR0;
    for(size_t i = 0; i < NKEYS; ++i)
    {
        union key_u k = {.key = config->keys[i]};
        tmp[i        ] = k.data[0];
        tmp[i + NKEYS] = k.data[1];
    }
    i2c_write(tmp, 2 * NKEYS + 1);
    
    // Now, enable interrupts on /CHANGE signal
    gpio_configure_function(config->change_port, config->change_pin, GPIO_FUNC_A);
}

uint8_t atqt2120_read_status(void)
{
    uint8_t tmp[3];
    i2c_read(ADD_DETECTION_STATUS, tmp, 3);
    return tmp[2];
}

