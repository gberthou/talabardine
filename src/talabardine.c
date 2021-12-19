#include "config.h"
#include "gclk.h"
#include "gpio.h"
#include "pm.h"
#include "sysctrl.h"
#include "sercom.h"
#include "atqt2120.h"
#include "abp-spi.h"
#include "nvic.h"
#include "eic.h"
#include "interrupt.h"
#include "tc.h"
#include "nvmctrl.h"
#include "midi.h"
#include "udc.h"
#include "usb.h"
#include "usb_midi.h"
#include "usb_talabardine.h"
#include "utils.h"

#define EIC_KEYCHANGE 5

#define PRESSURE_OCT1 ABP_PA_2_COUNTS(2000)
#define PRESSURE_OCT2 ABP_PA_2_COUNTS(4000)

#define OCTAVE_OFFSET 4

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

// KEY_DRTHR must be high enough so that keys are not triggered from the backside of the PCB
#define KEY_DTHR 64
const struct atqt2120_t keys_config = {
    .keys = {
        {.ctrl.bits = {.en = 0, .gpo = 0, .aks = 0, .guard = 0}, .dthr = KEY_DTHR},
        {.ctrl.bits = {.en = 0, .gpo = 0, .aks = 0, .guard = 0}, .dthr = KEY_DTHR},
        {.ctrl.bits = {.en = 0, .gpo = 0, .aks = 0, .guard = 0}, .dthr = KEY_DTHR},
        {.ctrl.bits = {.en = 0, .gpo = 0, .aks = 0, .guard = 0}, .dthr = KEY_DTHR},
        {.ctrl.bits = {.en = 0, .gpo = 0, .aks = 0, .guard = 0}, .dthr = KEY_DTHR},
        {.ctrl.bits = {.en = 0, .gpo = 0, .aks = 0, .guard = 0}, .dthr = KEY_DTHR},
        {.ctrl.bits = {.en = 0, .gpo = 0, .aks = 0, .guard = 0}, .dthr = KEY_DTHR},
        {.ctrl.bits = {.en = 1, .gpo = 0, .aks = 0, .guard = 0}, .dthr = KEY_DTHR},
        {.ctrl.bits = {.en = 1, .gpo = 0, .aks = 0, .guard = 0}, .dthr = KEY_DTHR},
        {.ctrl.bits = {.en = 1, .gpo = 0, .aks = 0, .guard = 0}, .dthr = KEY_DTHR},
        {.ctrl.bits = {.en = 1, .gpo = 0, .aks = 0, .guard = 0}, .dthr = KEY_DTHR},
        {.ctrl.bits = {.en = 1, .gpo = 0, .aks = 0, .guard = 0}, .dthr = KEY_DTHR}
    },
    .change_port = GPIO_PORT_B,
    .change_pin = 5,
    .di = 1
};

static int keys_to_note(uint8_t keys, unsigned int *octave_modifier)
{
    switch(keys & 0x7f)
    {
        case 0x00:
        case 0x40:
            *octave_modifier = 0;
            return MIDI_NOTE_A;
        case 0x01:
        case 0x41:
            *octave_modifier = 0;
            return MIDI_NOTE_G;
        case 0x02:
        case 0x06:
        case 0x42:
        case 0x46:
            *octave_modifier = 0;
            return MIDI_NOTE_GS;
        case 0x03:
        case 0x43:
            *octave_modifier = 0;
            return MIDI_NOTE_F;
        case 0x07:
        case 0x47:
            *octave_modifier = 0;
            return MIDI_NOTE_DS;
        case 0x0f:
            *octave_modifier = 0;
            return MIDI_NOTE_D;
        case 0x1f:
            *octave_modifier = 0;
            return MIDI_NOTE_C;
        case 0x3e:
        case 0x7e:
            *octave_modifier = 0;
            return MIDI_NOTE_AS;
        case 0x3f:
            *octave_modifier = -1;
            return MIDI_NOTE_AS;
        case 0x7f:
            *octave_modifier = -1;
            return MIDI_NOTE_A;
        default:
            *octave_modifier = 0;
            return -1;
    }
}

static int note_to_midi_key(int note, unsigned int octave)
{
    return MIDI_NOTE_TO_KEY(note, OCTAVE_OFFSET - 1 + octave);
}

static void replace_note(int note)
{
    static int current_note = -1;
    //uint8_t command[7];

    // If nothing to replace... skip
    if(note == current_note)
        return;

    //uint8_t *ptr;
    // 1. Mute the former note, if any
    if(current_note != -1)
    {
        //ptr = midi_note_off(command, 0, current_note, 64);
        usb_midi_note_off(0, note, 64);
    }
    //else
    //    ptr = command;

    // 2. Play the new note, if any
    if(note != -1)
    {
        //ptr = midi_note_on(ptr, 0, note, 64);
        usb_midi_note_on(0, note, 64);
    }

    // 3. Finalize string
    //*ptr = 0;

    // 4. Send command
    //sercom_usart_puts(SERCOM_MIDI, (char*) command);

    // 5. Update current_note
    current_note = note;
}

static void talabardine_init_gpios(void)
{
    // Codec (SERCOM 0)
    //gpio_configure_function(GPIO_PORT_A, 8, GPIO_FUNC_C);
    //gpio_configure_function(GPIO_PORT_A, 9, GPIO_FUNC_C);
    //gpio_configure_function(GPIO_PORT_A, 10, GPIO_FUNC_C);
    //gpio_configure_function(GPIO_PORT_A, 11, GPIO_FUNC_C);

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
    // /CHANGE signal is handled by atqt2120_init

    // LED
    gpio_configure_io(GPIO_PORT_B, 7, true);
    gpio_set_output(GPIO_PORT_B, 7, false);

    // Button
    gpio_configure_io(GPIO_PORT_A, 8, false);
    gpio_enable_pull(GPIO_PORT_A, 8, true);

    // USB
    gpio_configure_function(GPIO_PORT_A, 24, GPIO_FUNC_G);
    gpio_configure_function(GPIO_PORT_A, 25, GPIO_FUNC_G);
}

static void talabardine_init_sercoms(void)
{
    sercom_init_usart(SERCOM_MIDI_CHANNEL, USART_TXPO_PAD0, USART_RXPO_DISABLE, /*31250*/ 115200);
    sercom_init_spi_master(SERCOM_PRESSURE_CHANNEL, SPI_OUT_PAD312, SPI_IN_PAD0, 800000);
    sercom_init_i2c_master(SERCOM_KEYS_CHANNEL, 400000);
}

static uint8_t keys;
static uint8_t octave;

void talabardine_init(void)
{
    sysctrl_init_DFLL48M();

    // 37.12: NVM needs at least 1 wait state @48MHz when Vcc > 2.7v
    nvmctrl_set_wait_states(1);
    gclk_set_frequency(GCLK0, 48000000); // Main clock @48MHz

    pm_enable_APB_clock(PM_CLK_SERCOM0, true);
    pm_enable_APB_clock(PM_CLK_SERCOM1, true);
    pm_enable_APB_clock(PM_CLK_SERCOM2, true);
    pm_enable_APB_clock(PM_CLK_SERCOM4, true);
    pm_enable_APB_clock(PM_CLK_TC3, true);

    talabardine_init_gpios();
    talabardine_init_sercoms();
    
    gpio_set_output(GPIO_PORT_B, 7, true);
    sercom_usart_puts(SERCOM_MIDI_CHANNEL, "Press button to start");
    for(size_t i = 0; gpio_read(GPIO_PORT_A, 8); ++i)
    {
        sercom_usart_putc(SERCOM_MIDI_CHANNEL, (i & 1) ? '\b' : '_');
        for(size_t j = 0; j < 5000000; ++j)
            asm volatile("nop");
    }
    gpio_set_output(GPIO_PORT_B, 7, false);
    
    nvic_enable(NVIC_SERCOM0 + SERCOM_KEYS_CHANNEL);
    interrupt_enable();
    atqt2120_init(&keys_config);
    keys = atqt2120_read_status();
    octave = 0;

    /* Normally, EIC has higher (lower value) priority than SERCOM4 (keys), so we have to reverse
     * priority in order, for the key handler, to be able to use interrupt-based I2C.
     * Priority bits 5:0 are ignored so it starts with 64.
     */
    nvic_set_priority(NVIC_EIC, 64);
    eic_init();
    eic_enable(EIC_KEYCHANGE);
    nvic_enable(NVIC_EIC);
    
    tc_init(TC3, GCLK0, 1000);
    nvic_enable(NVIC_TC3);

    usb_talabardine_init();
    usb_midi_init();
    nvic_enable(NVIC_USB);
}

void keychange_handler(void)
{
    static int t = 1;
    
    eic_clear(EIC_KEYCHANGE);
    nvic_clear(NVIC_EIC);
    
    gpio_set_output(GPIO_PORT_B, 7, t);
    t = !t;

    uint8_t new_keys = atqt2120_read_status(); // Reads AND acknowledges the interrupt on the ATQT2120 side
    if(octave > 0) // Was playing
    {
        unsigned int octave_modifier;
        int new_note = keys_to_note(new_keys, &octave_modifier);
        if(new_note >= 0)
            new_note = note_to_midi_key(new_note, octave + octave_modifier);
        else
            dump(&new_keys, 1);
        replace_note(new_note);
    }
    keys = new_keys;
}

void pressure_handler(void)
{
    tc_clear_interrupt(TC3);
    nvic_clear(NVIC_TC3);

    uint16_t new_pressure = abp_wait_until_valid_pressure();
    if(new_pressure >= PRESSURE_OCT1)
    {
        uint8_t new_octave = (new_pressure >= PRESSURE_OCT2 ? 2 : 1);
        if(new_octave != octave)
        {
            unsigned int octave_modifier;
            int new_note = keys_to_note(keys, &octave_modifier);
            new_note = note_to_midi_key(new_note, new_octave + octave_modifier);
            replace_note(new_note);
            octave = new_octave;
        }
    }
    else
    {
        octave = 0;
        replace_note(-1);
    }
}

void sercom4_handler(void) // keys i2c
{
    nvic_clear(NVIC_SERCOM0 + 4);
    sercom_i2c_interrupt(4);
}

