#ifndef UDC_H
#define UDC_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "usb_common.h"

enum endpoint_direction_e
{
    ENDPOINT_OUT,
    ENDPOINT_IN
};

struct udc_control_callback
{
    enum
    {
        UDC_CONTROL_NONE,
        UDC_CONTROL_SET_ADDRESS,
        UDC_CONTROL_SET_CONFIG
    } type;
    uint16_t wValue;
    void (*callback)(const struct udc_control_callback *);
};

void udc_init(void);
void udc_attach(void);
bool udc_is_attached(void);
void udc_endpoint_configure(const struct usb_endpoint_descriptor_t *descriptor);
void udc_endpoint_unconfigure(void);
void udc_endpoint_set_buffer(uint8_t ep, enum endpoint_direction_e direction, volatile void *buffer);
void udc_tx(uint8_t ep, const void *data, size_t size);
void usb_rx(uint8_t ep, size_t size);
void udc_control_send(const struct udc_control_callback *cb);
void udc_set_address(uint8_t address);
void udc_stall(uint8_t ep);
void udc_register_receive_callback(uint8_t ep, void (*callback)(void));
void udc_register_send_callback(uint8_t ep, void (*callback)(void));
bool udc_is_suspended(void);

void udc_dump_endpoint(uint8_t ep);

#endif
