#ifndef SERCOM_H
#define SERCOM_H

#include <stdint.h>
#include <stddef.h>

#include "gpio.h"

enum usart_txpo_e
{
    USART_TXPO_PAD0 = 0,
    USART_TXPO_PAD2,
    USART_TXPO_PAD0_ALT,
    USART_TXPO_DISABLE
};

enum usart_rxpo_e
{
    USART_RXPO_PAD0 = 0,
    USART_RXPO_PAD1,
    USART_RXPO_PAD2,
    USART_RXPO_PAD3,
    USART_RXPO_DISABLE
};

enum spi_dopo_e
{
    // Number order: MOSI if master / MISO if slave, SCK, /CE
    SPI_OUT_PAD012 = 0,
    SPI_OUT_PAD231,
    SPI_OUT_PAD312,
    SPI_OUT_PAD031
};

enum spi_dipo_e
{
    // MISO if master, MOSI if slave
    SPI_IN_PAD0 = 0,
    SPI_IN_PAD1,
    SPI_IN_PAD2,
    SPI_IN_PAD3 
};

// Assumes that GCLK0 is running at 48MHz
void sercom_init_usart(uint8_t channel, enum usart_txpo_e txpo, enum usart_rxpo_e rxpo, uint32_t baudrate_Hz);
void sercom_usart_putc(uint8_t channel, char c);
void sercom_usart_puts(uint8_t channel, const char *s);
void sercom_usart_display_half(uint16_t x);

void sercom_init_spi_master(uint8_t channel, enum spi_dopo_e dopo, enum spi_dipo_e dipo, uint32_t baudrate_Hz);
void sercom_spi_burst(uint8_t channel, enum gpio_port_e ce_port, uint8_t ce_pin, void *dst, const void *src, size_t size);

void sercom_init_i2c_master(uint8_t channel, uint32_t baudrate_Hz);
void sercom_i2c_write(uint8_t channel, uint16_t address, const uint8_t *data, size_t length);
void sercom_i2c_read(uint8_t channel, uint16_t address, uint8_t *data, size_t length);

#endif

