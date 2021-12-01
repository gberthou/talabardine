#ifndef USB_COMMON_H
#define USB_COMMON_H

#include <stdint.h>

struct __attribute__((packed)) usb_endpoint_descriptor_t
{
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bEndpointAddress;
    uint8_t bmAttributes;
    uint16_t wMaxPacketSize;
    uint8_t bInterval;
};

#endif

