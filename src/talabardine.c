#include "gclk.h"
#include "gpio.h"
#include "pm.h"
#include "sysctrl.h"
#include "sercom.h"

#define SERCOM_MIDI_CHANNEL 1
#define SERCOM_CODEC_CHANNEL 0 // or 2
#define SERCOM_PRESSURE_CHANNEL 2 
#define SERCOM_KEYS_CHANNEL 4

/*
 * Pin mapping:
 *     # USART (SERCOM 1)
 *         PA00 = TX
 *         PA01 = RX
 *     # Codec (SERCOM 0) // TODO
 *         PA08 = MISO
 *         PA09 = /CE
 *         PA10 = MOSI
 *         PA11 = SCK
 *     # Pressure (SERCOM 2) // TODO
 *         PA12 = MISO
 *         PA13 = /CE
 *         PA14 = MOSI
 *         PA15 = SCK
 *     # Keys (SERCOM 4)
 *         PB12 = SDA
 *         PB13 = SCL
 *         PB05 = /CHANGE
 *     # LEDs
 *         PB07
 */

static void talabardine_init_gpios(void)
{
    // Codec (SERCOM 0)
    /*
    gpio_configure_function(GPIO_PORT_A, 8, GPIO_FUNC_C);
    gpio_configure_function(GPIO_PORT_A, 9, GPIO_FUNC_C);
    gpio_configure_function(GPIO_PORT_A, 10, GPIO_FUNC_C);
    gpio_configure_function(GPIO_PORT_A, 11, GPIO_FUNC_C);
    */

    // USART (SERCOM 1)
    gpio_configure_function(GPIO_PORT_A, 0, GPIO_FUNC_D);
    gpio_configure_function(GPIO_PORT_A, 1, GPIO_FUNC_D);

    // Pressure (SERCOM 2)
    /*
    gpio_configure_function(GPIO_PORT_A, 12, GPIO_FUNC_C);
    gpio_configure_io      (GPIO_PORT_A, 13, true); // /CE must be manual because of SPI bursts
    gpio_set_output        (GPIO_PORT_A, 13, true);
    gpio_configure_function(GPIO_PORT_A, 14, GPIO_FUNC_C);
    gpio_configure_function(GPIO_PORT_A, 15, GPIO_FUNC_C);
    */

    // Keys (SERCOM 4)
    gpio_configure_function(GPIO_PORT_B, 12, GPIO_FUNC_C);
    gpio_configure_function(GPIO_PORT_B, 13, GPIO_FUNC_C);
    gpio_configure_io(GPIO_PORT_B, 5, false);

    // LED
    gpio_configure_io(GPIO_PORT_B, 7, true);
    gpio_set_output(GPIO_PORT_B, 7, false);
}

static void talabardine_init_sercoms(void)
{
    sercom_init_usart(SERCOM_MIDI_CHANNEL, USART_TXPO_PAD0, USART_RXPO_DISABLE, 115200);
    //sercom_init_spi_master(SERCOM_PRESSURE_CHANNEL, SPI_OUT_PAD231, SPI_IN_PAD0, 800000);
    sercom_init_i2c_master(SERCOM_KEYS_CHANNEL, 400000);
}

void talabardine_init(void)
{
    sysctrl_init_DFLL48M();
    gclk_main_48MHz();

    pm_enable_APB_clock(PM_CLK_SERCOM0, true);
    pm_enable_APB_clock(PM_CLK_SERCOM1, true);
    pm_enable_APB_clock(PM_CLK_SERCOM2, true);
    pm_enable_APB_clock(PM_CLK_SERCOM4, true);

    talabardine_init_gpios();
    talabardine_init_sercoms();
    sercom_usart_puts(SERCOM_MIDI_CHANNEL, "Salut !\r\nJe suis un programme\r\n");

    void atqt2120_test(void);
    atqt2120_test();
}

uint16_t talabardine_get_pressure(void)
{
    /*
    uint16_t tx;
    uint16_t rx;
    sercom_spi_burst(SERCOM_PRESSURE_CHANNEL, GPIO_PORT_A, 13, &rx, &tx, sizeof(rx));
    return rx;
    */
    return 0;
}
