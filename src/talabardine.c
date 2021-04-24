#include "config.h"
#include "gclk.h"
#include "gpio.h"
#include "pm.h"
#include "sysctrl.h"
#include "sercom.h"
#include "atqt2120.h"
#include "abp-spi.h"

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
 *     # Pressure (SERCOM 0)
 *         PA04 = MISO
 *         PA05 = SCK
 *         PA06 = /CE
 *     # Keys (SERCOM 4)
 *         PB12 = SDA
 *         PB13 = SCL
 *         PB05 = /CHANGE
 *     # LEDs
 *         PB07
 */

const struct atqt2120_t keys_config = {
    .keys = {
        {.en = 0, .gpo = 0, .aks = 0, .guard = 0},
        {.en = 0, .gpo = 0, .aks = 0, .guard = 0},
        {.en = 0, .gpo = 0, .aks = 0, .guard = 0},
        {.en = 0, .gpo = 0, .aks = 0, .guard = 0},
        {.en = 0, .gpo = 0, .aks = 0, .guard = 0},
        {.en = 0, .gpo = 0, .aks = 0, .guard = 0},
        {.en = 0, .gpo = 0, .aks = 0, .guard = 0},
        {.en = 0, .gpo = 0, .aks = 0, .guard = 0},
        {.en = 0, .gpo = 0, .aks = 0, .guard = 0},
        {.en = 0, .gpo = 0, .aks = 0, .guard = 0},
        {.en = 0, .gpo = 0, .aks = 0, .guard = 0},
        {.en = 0, .gpo = 0, .aks = 0, .guard = 0}
    },
    .change_port = GPIO_PORT_B,
    .change_pin = 5
};

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

    // Pressure (SERCOM 0)
    gpio_configure_function(GPIO_PORT_A, 4, GPIO_FUNC_D);
    gpio_configure_function(GPIO_PORT_A, 5, GPIO_FUNC_D);
    gpio_configure_io      (GPIO_PORT_A, 6, true); // /CE must be manual because of SPI bursts
    gpio_set_output        (GPIO_PORT_A, 6, true);

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
    sercom_init_spi_master(SERCOM_PRESSURE_CHANNEL, SPI_OUT_PAD312, SPI_IN_PAD0, 800000);
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

    atqt2120_init(&keys_config);
    for(;;)
    {
        while(gpio_read(keys_config.change_port, keys_config.change_pin))
        {
            uint16_t p = abp_get_pressure();
            sercom_usart_display_half(p);
        }
        uint8_t status = atqt2120_read_status();
        sercom_usart_display_half(status);
    }
}

