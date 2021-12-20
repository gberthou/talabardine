#ifndef DRIVERS_USB_MIDI_H
#define DRIVERS_USB_MIDI_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "midi.h"

struct __attribute__((packed)) cs_ac_interface_descriptor_t
{
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bDescriptorSubtype;
    uint16_t bcdADC;
    uint16_t wTotalLength;
    uint8_t bInCollection;
    uint8_t baInterfaceNr;
};

struct __attribute__((packed)) cs_ms_interface_descriptor_t
{
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bDescriptorSubtype;
    uint16_t bcdMSC;
    uint16_t wTotalLength;
};

struct __attribute__((packed)) cs_ms_midi_in_descriptor_t
{
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bDescriptorSubtype;
    uint8_t bJackType;
    uint8_t bJackID;
    uint8_t iJack;
};

struct __attribute__((packed)) cs_ms_midi_out_endpoint_t
{
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bDescriptorSubtype;
    uint8_t bJackType;
    uint8_t bJackID;
    uint8_t bNrInputPins;
    uint8_t baSourceID;
    uint8_t baSourcePin;
    uint8_t iJack;
};

struct __attribute__((packed)) bulk_endpoint_descriptor_t
{
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bEndpointAddress;
    uint8_t bmAttributes;
    uint16_t wMaxPacketSize;
    uint8_t bInterval;
    uint8_t bRefresh;
    uint8_t bSynchAddress;
};

struct __attribute__((packed)) cs_ms_bulk_endpoint_descriptor_t
{
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bDescriptorSubtype;
    uint8_t bNumEmbMIDIJack;
    uint8_t baAssocJackID;
};

void usb_midi_init(void);
bool usb_midi_note_off(uint8_t channel, uint8_t key, uint8_t velocity);
bool usb_midi_note_on(uint8_t channel, uint8_t key, uint8_t velocity);

#endif

