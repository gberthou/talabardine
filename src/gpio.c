#include <stddef.h>

#include "gpio.h"

struct __attribute__((packed)) gpio_t
{
    uint32_t dir;
    uint32_t dirclr;
    uint32_t dirset;
    uint32_t dirtgl;
    uint32_t out;
    uint32_t outclr;
    uint32_t outset;
    uint32_t outtgl;
    uint32_t in;
    uint32_t ctrl;
    uint32_t wrconfig;
    uint32_t _pad0;
    uint8_t pmux[16];
    uint8_t pincfg[32];
};

#define PORT0 0x41004400
#define PORT(i) ((volatile struct gpio_t*) (PORT0 + ((i) << 7)))

/*
 * 23.5.3:
 * The PORT requires an APB clock, which may be divided from the CPU main clock and allows
 * the CPU to access the registers of PORT through the high-speed matrix and the AHB/APB
 * bridge.
 * The PORT also requires an AHB clock for CPU IOBUS accesses to the PORT. That AHB clock is
 * the internal PORT clock.
 *
 * Table 16-1 says that CLK_PORT_APB is enabled on reset
 */

void gpio_configure_function(enum gpio_port_e port, uint8_t pin, enum gpio_function_e function)
{
    pin &= 0x1f;

    // 23.6.2.2: The chosen peripheral must also be configured and enabled.
    
    // 1. Set PMUXEN to enable special functions
    volatile struct gpio_t *p = PORT(port);
    p->pincfg[pin] =  1 // PMUXEN
                   | (0 << 1) // INEN (input buffer disabled)
                   | (0 << 2) // PULLEN (no pull resistor)
                   | (0 << 6) // DRVSTR (normal strength)
                   ;

    // 2. Configure PMUXO/PMUXE to select the function
    volatile uint8_t *pmux = &p->pmux[pin >> 1];
    uint8_t tmp = *pmux;
    size_t shift = ((pin & 1) << 2); // 0 if pin is even, 4 otherwise
    tmp &= ~(0xf << shift);
    tmp |= (function << shift);
    *pmux = tmp;
}

void gpio_configure_io(enum gpio_port_e port, uint8_t pin, bool output)
{
    pin &= 0x1f;

    volatile struct gpio_t *p = PORT(port);
    p->pincfg[pin] =  0 // PMUXEN
                   | (output ? 0 : (1 << 1)) // INEN (input buffer disabled)
                   | (0 << 2) // PULLEN (no pull resistor)
                   | (0 << 6) // DRVSTR (normal strength)
                   ;

    uint32_t mask =(1 << pin);
    if(output)
        p->dirset = mask;
    else
        p->dirclr = mask;
}

void gpio_set_output(enum gpio_port_e port, uint8_t pin, bool high)
{
    pin &= 0x1f;

    volatile struct gpio_t *p = PORT(port);
    uint32_t mask = (1 << pin);
    if(high)
        p->outset = mask;
    else
        p->outclr = mask;
}

bool gpio_read(enum gpio_port_e port, uint8_t pin)
{
    pin &= 0x1f;

    volatile struct gpio_t *p = PORT(port);
    uint32_t mask = (1 << pin);
    return (p->in & mask) != 0;
}

