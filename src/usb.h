#ifndef USB_H
#define USB_H

#include <stdint.h>
#include <stdbool.h>

#define USB_STRLEN(x) (sizeof(x) / sizeof((x)[0]))

#define USB_LANGID_DESCRIPTOR_T(x) \
struct __attribute__((packed)) \
{ \
    uint8_t bLength; \
    uint8_t bDescriptorType; \
    uint16_t wLANGID[x]; \
}
#define USB_UNICODE_DESCRIPTOR_T(name, s) \
struct __attribute__((packed, aligned(4))) \
{ \
    uint8_t bLength; \
    uint8_t bDescriptorType; \
    uint16_t bString[USB_STRLEN(s)]; \
} name = { \
    .bLength = 2 + sizeof(s), \
    .bDescriptorType = 0x3, \
    .bString = s \
}

struct __attribute__((packed)) usb_device_descriptor_t
{
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t bcdUSB;
    uint8_t bDeviceClass;
    uint8_t bDeviceSubClass;
    uint8_t bDeviceProtocol;
    uint8_t bMaxPacketSize;
    uint16_t idVendor;
    uint16_t idProduct;
    uint16_t bcdDevice;
    uint8_t iManufacturer;
    uint8_t iProduct;
    uint8_t iSerialNumber;
    uint8_t bNumConfigurations;
};

struct __attribute__((packed)) usb_configuration_descriptor_t
{
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t wTotalLength;
    uint8_t bNumInterfaces;
    uint8_t bConfigurationValue;
    uint8_t iConfiguration;
    uint8_t bmAttributes;
    uint8_t bMaxPower;
};

struct __attribute__((packed)) usb_interface_descriptor_t
{
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bInterfaceNumber;
    uint8_t bAlternateSetting;
    uint8_t bNumEndpoints;
    uint8_t bInterfaceClass;
    uint8_t bInterfaceSubClass;
    uint8_t bInterfaceProtocol;
    uint8_t iInterface;
};

struct __attribute__((packed)) usb_qualifier_descriptor_t
{
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t bcdUSB;
    uint8_t bDeviceClass;
    uint8_t bDeviceSubClass;
    uint8_t bDeviceProtocol;
    uint8_t bMaxPacketSize0;
    uint8_t bNumConfigurations;
    uint8_t bReserved;
};

typedef void (*usb_configuration_cb)(uint16_t);

void usb_setup_packet(const volatile void *_buf);
bool usb_is_configured(uint16_t config);
void usb_set_configuration_callback(usb_configuration_cb callback);
void usb_set_device_descriptor(const struct usb_device_descriptor_t *descriptor);
bool usb_set_configuration_descriptor(uint16_t config, const void *descriptor);
bool usb_set_string_descriptor(uint16_t id, const void *descriptor);
bool usb_set_qualifier_descriptor(uint16_t id, const struct usb_qualifier_descriptor_t *descriptor);

#endif

