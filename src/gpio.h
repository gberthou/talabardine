#ifndef GPIO_H
#define GPIO_H

#include <stdint.h>
#include <stdbool.h>

enum gpio_port_e
{
    GPIO_PORT_A = 0,
    GPIO_PORT_B
};

enum gpio_function_e
{
    GPIO_FUNC_A = 0,
    GPIO_FUNC_B,
    GPIO_FUNC_C,
    GPIO_FUNC_D,
    GPIO_FUNC_E,
    GPIO_FUNC_F,
    GPIO_FUNC_G,
    GPIO_FUNC_H,
    GPIO_FUNC_I
};

void gpio_configure_function(enum gpio_port_e port, uint8_t pin, enum gpio_function_e function);
void gpio_configure_io(enum gpio_port_e port, uint8_t pin, bool output);
void gpio_set_output(enum gpio_port_e port, uint8_t pin, bool high);
bool gpio_read(enum gpio_port_e port, uint8_t pin);

#endif

