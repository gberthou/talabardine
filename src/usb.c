#include <stdint.h>
#include <stddef.h>

#include "usb.h"
#include "udc.h"
#include "usb_common.h"
#include "sercom.h"
#include "config.h"

#define STRLEN(x) (sizeof(x) / sizeof((x)[0]))

// https://www.beyondlogic.org/usbnutshell/usb4.shtml
// https://www.beyondlogic.org/usbnutshell/usb5.shtml#DeviceDescriptors
// https://www.beyondlogic.org/usbnutshell/usb6.shtml#SetupPacket

struct __attribute__((packed)) usb_setup_packet_t
{
    uint8_t bmRequestType;
    uint8_t bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
};

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
    uint16_t bString[STRLEN(s)]; \
} name = { \
    .bLength = 2 + sizeof(s), \
    .bDescriptorType = 0x3, \
    .bString = s \
}

const struct usb_device_descriptor_t __attribute__((aligned(4))) TALABARDINE_USB_DESCRIPTOR = {
    .bLength = 18,
    .bDescriptorType = 0x01, // Device descriptor
    .bcdUSB = 0x0200, // USB 2.0
    .bDeviceClass = 0x00, // Interface-defined
    .bDeviceSubClass = 0x00,
    .bDeviceProtocol = 0x00,
    .bMaxPacketSize = 64, // 64 bytes
    .idVendor = 0xdead,
    .idProduct = 0xbeef,
    .bcdDevice = 0x0001,
    .iManufacturer = 1,
    .iProduct = 2,
    .iSerialNumber = 3,
    .bNumConfigurations = 1 // 1 configuration
};
const struct __attribute__((packed, aligned(4)))
{
    struct usb_configuration_descriptor_t configuration;
    struct usb_interface_descriptor_t interfaces[2];
    struct usb_endpoint_descriptor_t endpoints[2];
} TALABARDINE_CONFIG0_DESCRIPTOR = {
    .configuration = {
        .bLength = 9,
        .bDescriptorType = 0x02, // Configuration Descriptor
        .wTotalLength = 41,
        .bNumInterfaces = 2,
        .bConfigurationValue = 1,
        .iConfiguration = 0,
        .bmAttributes = 0xa0, // D7 (reserved) | Remote wakeup
        .bMaxPower = 50 // 100 mA max.
    },

    .interfaces[0] = { // Standard AC interface descriptor
        .bLength = 9,
        .bDescriptorType = 0x04, // Interface Descriptor
        .bInterfaceNumber = 0,
        .bAlternateSetting = 0,
        .bNumEndpoints = 0, // No endpoints
        .bInterfaceClass = 0x01, // Audio
        .bInterfaceSubClass = 0x01, // Audio control
        .bInterfaceProtocol = 0, // Unused
        .iInterface = 0
    },
    .interfaces[1] = { // Standard MIDIStreaming descriptor
        .bLength = 9,
        .bDescriptorType = 0x04, // Interface Descriptor
        .bInterfaceNumber = 1,
        .bAlternateSetting = 0,
        .bNumEndpoints = 2,
        .bInterfaceClass = 0x01, // Audio
        .bInterfaceSubClass = 0x03, // MIDIStreaming
        .bInterfaceProtocol = 0,
        .iInterface = 0
    },

    .endpoints[0] = {
        .bLength = 7,
        .bDescriptorType = 0x05, // Endpoint Descriptor
        .bEndpointAddress = 0x01, // Endpoint 1 OUT
        .bmAttributes = 0x03, // Interrupt
        .wMaxPacketSize = 64,
        .bInterval = 1
    },
    .endpoints[1] = {
        .bLength = 7,
        .bDescriptorType = 0x05, // Endpoint Descriptor
        .bEndpointAddress = 0x81, // Endpoint 1 IN
        .bmAttributes = 0x03, // Interrupt
        .wMaxPacketSize = 64,
        .bInterval = 1
    }
};
const struct __attribute__((aligned(4))) usb_qualifier_descriptor_t TALABARDINE_QUALIFIER_DESCRIPTOR = {
    .bLength = 10,
    .bDescriptorType = 0x6, // Device Qualifier Descriptor
    .bcdUSB = TALABARDINE_USB_DESCRIPTOR.bcdUSB,
    .bDeviceClass = TALABARDINE_USB_DESCRIPTOR.bDeviceClass,
    .bDeviceSubClass = TALABARDINE_USB_DESCRIPTOR.bDeviceSubClass,
    .bDeviceProtocol = TALABARDINE_USB_DESCRIPTOR.bDeviceProtocol,
    .bMaxPacketSize0 = 64,
    .bNumConfigurations = 0,
    .bReserved = 0
};
const USB_LANGID_DESCRIPTOR_T(1) TALABARDINE_LANGID_DESCRIPTOR = {
    .bLength = sizeof(USB_LANGID_DESCRIPTOR_T(1)),
    .bDescriptorType = 0x03, // String Descriptor
    .wLANGID[0] = 0x0409 // en_US
};
const USB_UNICODE_DESCRIPTOR_T(TALABARDINE_MANUFACTURER, u"gberthou");

static void dump(const void *_src, size_t size)
{
    const unsigned char *src = _src;
    for(size_t j = 0; j < size; ++j)
    {
        unsigned char c = *src++;
        char repr[2];
        for(size_t i = 0; i < 2; ++i)
        {
            unsigned char tmp = (c & 0xf);
            if(tmp < 10)
                tmp += '0';
            else
                tmp += 'a' - 10;
            repr[i^1] = tmp;
            c >>= 4;
        }
        sercom_usart_putc(SERCOM_MIDI_CHANNEL, repr[0]); 
        sercom_usart_putc(SERCOM_MIDI_CHANNEL, repr[1]); 
        if((j & 7) != 7)
            sercom_usart_putc(SERCOM_MIDI_CHANNEL, ' '); 
        else
        {
            sercom_usart_putc(SERCOM_MIDI_CHANNEL, '\r'); 
            sercom_usart_putc(SERCOM_MIDI_CHANNEL, '\n'); 
        }
    }
}

void usb_setup_packet(const void *_buf)
{
    const struct usb_setup_packet_t *packet = _buf;
    uint16_t length = packet->wLength;

    // GET
    if(packet->bmRequestType == 0x80)
    {
        uint16_t type = (packet->wValue >> 8);
        uint16_t id   = (packet->wValue & 0xff);
        switch(packet->bRequest)
        {
            case 0x06: //GET_DESCRIPTOR
                switch(type)
                {
                    case 0x01: // DEVICE
                        if(id == 0)
                            udc_tx(0, &TALABARDINE_USB_DESCRIPTOR, sizeof(TALABARDINE_USB_DESCRIPTOR));
                        else
                            udc_stall();
                        break;
                    case 0x02: // CONFIGURATION
                        if(id == 0)
                        {
                            /* /!\ Here, we must not send the entire
                             * configuration descriptor. First, the host asks
                             * for the configuration itself (9 bytes). Then,
                             * the host will ask again for the configuration,
                             * but this time with wLength equal to wTotalLength
                             * from the previous configuration response sent by
                             * the device
                             */
                            /* Also, we might want to reduce buffer overflow
                             * risks, why should we trust the host after all?
                             */
                            size_t size = (length <= TALABARDINE_CONFIG0_DESCRIPTOR.configuration.wTotalLength ? length : TALABARDINE_CONFIG0_DESCRIPTOR.configuration.wTotalLength);
                            udc_tx(0, &TALABARDINE_CONFIG0_DESCRIPTOR, size);
                        }
                        else
                            udc_stall();
                        break;
                    case 0x03: // STRING
                        if(id == 0)
                            udc_tx(0, &TALABARDINE_LANGID_DESCRIPTOR, sizeof(TALABARDINE_LANGID_DESCRIPTOR));
                        else if(id <= 3)
                            udc_tx(0, &TALABARDINE_MANUFACTURER, sizeof(TALABARDINE_MANUFACTURER));
                        else
                            udc_stall();
                        break;
                    case 0x06: // DEVICE QUALIFIER
                        if(id == 0)
                            udc_tx(0, &TALABARDINE_QUALIFIER_DESCRIPTOR, sizeof(TALABARDINE_QUALIFIER_DESCRIPTOR));
                        else
                            udc_stall();
                        break;

                    default:
                        sercom_usart_puts(SERCOM_MIDI_CHANNEL, "GD ");
                        dump(&packet->wValue, sizeof(packet->wValue));
                        udc_stall();
                }
                break;

            default:
                sercom_usart_puts(SERCOM_MIDI_CHANNEL, "[GET] Unknown SETUP.bRequest ");
                dump(&packet->bRequest, sizeof(packet->bRequest));
                sercom_usart_puts(SERCOM_MIDI_CHANNEL, "\r\n");
        }
    }
    // SET
    else if(packet->bmRequestType == 0x00)
    {
        switch(packet->bRequest)
        {
            case 0x05: // SET_ADDRESS
            {
                uint16_t address = packet->wValue;
                udc_control_send();
                udc_set_address(address);
                break;
            }

            case 0x09: // SET_CONFIGURATION
            {
                uint16_t config = packet->wValue;
                udc_control_send();
                if(!config)
                    udc_endpoint_unconfigure();
                else
                {
                    const uint8_t *ptr = (uint8_t*)&TALABARDINE_CONFIG0_DESCRIPTOR;
                    const uint8_t *end = ptr + TALABARDINE_CONFIG0_DESCRIPTOR.configuration.wTotalLength;
                    for(ptr += ptr[0]; ptr < end; ptr += ptr[0])
                        if(ptr[1] == 0x04) // Interface descriptor
                            udc_endpoint_configure((void*)ptr);
                }
                break;
            }

            default:
                sercom_usart_puts(SERCOM_MIDI_CHANNEL, "[SET] Unknown SETUP.bRequest ");
                dump(&packet->bRequest, sizeof(packet->bRequest));
                sercom_usart_puts(SERCOM_MIDI_CHANNEL, "\r\n");
        }
    }
    else
        sercom_usart_puts(SERCOM_MIDI_CHANNEL, "OOPS\r\n");
}

