#include <string.h>

#include "usb_midi.h"
#include "udc.h"
#include "usb.h"

static bool tx(const void *data, size_t size)
{
    if(udc_is_attached() && !udc_is_suspended() && usb_is_configured(1))
    {
        udc_busy_wait_tx_free(1);
        udc_tx(1, data, size);
        return true;
    }
    return false;
}

bool usb_midi_note_off(uint8_t channel, uint8_t key, uint8_t velocity)
{
    uint8_t __attribute__((aligned(4))) tmp[4] = {0x09};
    midi_note_off(tmp + 1, channel, key, velocity);
    return tx(tmp, sizeof(tmp));
}

bool usb_midi_note_on(uint8_t channel, uint8_t key, uint8_t velocity)
{
    uint8_t __attribute__((aligned(4))) tmp[4] = {0x09};
    midi_note_on(tmp + 1, channel, key, velocity);
    return tx(tmp, sizeof(tmp));
}

