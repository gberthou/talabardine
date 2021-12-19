#include "atqt2120.h"
#include "sercom.h"
#include "config.h"

#define SERCOMID SERCOM_KEYS_CHANNEL
#define ATQT2120_I2C_ADDRESS 0x1c

#define NKEYS 12

#define ADD_DETECTION_STATUS 2
#define ADD_CALIBRATE 6
#define ADD_RESET 7
#define ADD_TTD 9
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
    while(!gpio_read(config->change_port, config->change_pin));
    atqt2120_read_status();
    
    // 2. Set DI (lower = higher time-related reactivity)
    tmp[0] = ADD_DI;
    tmp[1] = config->di;
    i2c_write(tmp, 2);
    
    tmp[0] = ADD_TTD;
    tmp[1] = 1; // TTD
    tmp[2] = 1; // ATD
    i2c_write(tmp, 3);

    // 3. Configure keys
    tmp[0] = ADD_DTHR0;
    for(size_t i = 0; i < NKEYS; ++i)
    {
        tmp[i+1        ] = config->keys[i].dthr;
        tmp[i+1 + NKEYS] = config->keys[i].ctrl.raw;
    }
    i2c_write(tmp, 2 * NKEYS + 1);
    
    // 4. Calibrate
    tmp[0] = ADD_CALIBRATE;
    tmp[1] = 1;
    i2c_write(tmp, 2);
    
    // Now, enable interrupts on /CHANGE signal
    gpio_configure_function(config->change_port, config->change_pin, GPIO_FUNC_A);
}

uint8_t atqt2120_read_status(void)
{
    uint8_t tmp[3];
    i2c_read(ADD_DETECTION_STATUS, tmp, 3);
    return tmp[1];
}

