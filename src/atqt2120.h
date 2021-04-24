#ifndef ATQT2120_h
#define ATQT2120_h

#include <stdint.h>
#include "gpio.h"

struct __attribute__((packed)) atqt2120_key_t
{
    uint8_t en    : 1;
    uint8_t gpo   : 1;
    uint8_t aks   : 2;
    uint8_t guard : 1;

    uint8_t dthr;
};

struct atqt2120_t
{
    struct atqt2120_key_t keys[12];
    enum gpio_port_e change_port;
    uint8_t change_pin;
    uint8_t di;
};

void atqt2120_init(const struct atqt2120_t *config);

// Only returns the status of KEY0 to KEY7
uint8_t atqt2120_read_status(void);

#endif

