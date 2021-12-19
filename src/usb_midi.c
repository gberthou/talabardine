#include <string.h>

#include "usb_midi.h"
#include "udc.h"
#include "usb.h"
#include "interrupt.h"
#include "config.h"

#define MIDI1_IN_ENDPOINT 1

#define MAX_PACKET_SIZE 4
#define N_PACKETS 64

struct usb_midi_packet_t
{
    uint8_t data[MAX_PACKET_SIZE];
    uint8_t size;
};

static struct
{
    struct usb_midi_packet_t packets[N_PACKETS];
    struct usb_midi_packet_t * volatile current;
    struct usb_midi_packet_t * volatile stack;
} context;

static bool tx(const void *data, size_t size)
{
    if(!udc_is_attached() || udc_is_suspended() || !usb_is_configured(MIDI1_IN_ENDPOINT))
        return false;

    interrupt_disable();
    
    if(context.current == context.stack)
         udc_tx(MIDI1_IN_ENDPOINT, data, size);
    else
    {
        memcpy(context.stack, data, size);
        context.stack->size = size;
        if(++context.stack >= context.packets + N_PACKETS)
            context.stack = context.packets;
    }
    
    interrupt_enable();
    return true;
}

static void usb_midi_send_callback(void)
{
    interrupt_disable();
    
    if(context.current != context.stack)
    {
        udc_tx(MIDI1_IN_ENDPOINT, context.current->data, context.current->size);
        if(++context.current >= context.packets + N_PACKETS)
            context.current = context.packets;
    }
    
    interrupt_enable();
}

void usb_midi_init(void)
{
    context.current = context.packets;
    context.stack = context.packets;
    
    udc_register_send_callback(MIDI1_IN_ENDPOINT, usb_midi_send_callback);
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
