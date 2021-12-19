#ifndef DRIVERS_USB_MIDI_H
#define DRIVERS_USB_MIDI_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "midi.h"

void usb_midi_init(void);
bool usb_midi_note_off(uint8_t channel, uint8_t key, uint8_t velocity);
bool usb_midi_note_on(uint8_t channel, uint8_t key, uint8_t velocity);

#endif

