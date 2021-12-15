#include <stdint.h>
#include <stddef.h>

#include "usb.h"
#include "udc.h"
#include "usb_common.h"
#include "sercom.h"
#include "config.h"
#include "utils.h"

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

    struct usb_interface_descriptor_t midi1_interface;
    struct cs_ac_interface_descriptor_t ac_interface;

    struct usb_interface_descriptor_t midi2_interface;
    struct cs_ms_interface_descriptor_t ms_interface;
    struct cs_ms_midi_in_descriptor_t midi_in_interfaces[2];
    struct cs_ms_midi_out_endpoint_t midi_out_interface_0;
    struct cs_ms_midi_out_endpoint_t midi_out_interface_1;
    struct bulk_endpoint_descriptor_t midi_bulk_in;
    struct cs_ms_bulk_endpoint_descriptor_t midi_cs_bulk_in;
    struct bulk_endpoint_descriptor_t midi_bulk_out;
    struct cs_ms_bulk_endpoint_descriptor_t midi_cs_bulk_out;

} TALABARDINE_CONFIG0_DESCRIPTOR = {
    .configuration = {
        .bLength = 9,
        .bDescriptorType = 0x02, // Configuration Descriptor
        .wTotalLength = 101,
        .bNumInterfaces = 2,
        .bConfigurationValue = 1,
        .iConfiguration = 0,
        .bmAttributes = 0xa0, // D7 (reserved) | Remote wakeup
        .bMaxPower = 50 // 100 mA max.
    },

    .midi1_interface = { // Standard AC interface descriptor
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
    .ac_interface = {
        .bLength = 9,
        .bDescriptorType = 0x24, // CS_INTERFACE
        .bDescriptorSubtype = 0x01, // HEADER
        .bcdADC = 0x0100, // Audio 1.0
        .wTotalLength = 9,
        .bInCollection = 1, // 1 streaming interface
        .baInterfaceNr = 1
    },

    .midi2_interface = { // Standard MIDIStreaming descriptor
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
    .ms_interface = {
        .bLength = 7,
        .bDescriptorType = 0x24, // CS_INTERFACE
        .bDescriptorSubtype = 0x01, // MS_HEADER
        .bcdMSC = 0x0100,
        .wTotalLength = 65
    },
    .midi_in_interfaces[0] = {
        .bLength = 6,
        .bDescriptorType = 0x24, // CS_INTERFACE
        .bDescriptorSubtype = 0x02, // MIDI_IN_JACK
        .bJackType = 0x1, // EMBEDDED
        .bJackID = 0x1,
        .iJack = 0x0
    },
    .midi_in_interfaces[1] = {
        .bLength = 6,
        .bDescriptorType = 0x24, // CS_INTERFACE
        .bDescriptorSubtype = 0x02, // MIDI_IN_JACK
        .bJackType = 0x2, // EXTERNAL
        .bJackID = 0x2,
        .iJack = 0x0
    },
    .midi_out_interface_0 = {
        .bLength = 9,
        .bDescriptorType = 0x24, // CS_INTERFACE
        .bDescriptorSubtype = 0x03, // MIDI_OUT_JACK
        .bJackType = 0x1, // EMBEDDED
        .bJackID = 0x3,
        .bNrInputPins = 1, // One input pin
        .baSourceID = 0x2,
        .baSourcePin = 0x1,
        .iJack = 0
    },
    .midi_out_interface_1 = {
        .bLength = 9,
        .bDescriptorType = 0x24, // CS_INTERFACE
        .bDescriptorSubtype = 0x03, // MIDI_OUT_JACK
        .bJackType = 0x2, // EXTERNAL
        .bJackID = 0x4,
        .bNrInputPins = 1, // One input pin
        .baSourceID = 0x1,
        .baSourcePin = 0x1,
        .iJack = 0
    },
    .midi_bulk_in = {
        .bLength = 9,
        .bDescriptorType = 0x5, // ENDPOINT descriptor
        .bEndpointAddress = 0x81, // IN 1
        .bmAttributes = 0x2, // Bulk, not shared
        .wMaxPacketSize = 64,
        .bInterval = 0,
        .bRefresh = 0,
        .bSynchAddress = 0
    },
    .midi_cs_bulk_in = {
        .bLength = 5,
        .bDescriptorType = 0x25, // CS_ENDPOINT
        .bDescriptorSubtype = 0x1, // MS_GENERAL
        .bNumEmbMIDIJack = 0x1,
        .baAssocJackID = 0x3
    },
    .midi_bulk_out = {
        .bLength = 9,
        .bDescriptorType = 0x5, // ENDPOINT descriptor
        .bEndpointAddress = 0x02, // OUT 2
        .bmAttributes = 0x2, // Bulk, not shared
        .wMaxPacketSize = 64,
        .bInterval = 0,
        .bRefresh = 0,
        .bSynchAddress = 0
    },
    .midi_cs_bulk_out = {
        .bLength = 5,
        .bDescriptorType = 0x25, // CS_ENDPOINT
        .bDescriptorSubtype = 0x1, // MS_GENERAL
        .bNumEmbMIDIJack = 0x1,
        .baAssocJackID = 0x1
    }
};
const struct __attribute__((aligned(4))) usb_qualifier_descriptor_t TALABARDINE_QUALIFIER_DESCRIPTOR = {
    .bLength = 10,
    .bDescriptorType = 0x6, // Device Qualifier Descriptor
    .bcdUSB = 0x0200, //TALABARDINE_USB_DESCRIPTOR.bcdUSB,
    .bDeviceClass = 0, //TALABARDINE_USB_DESCRIPTOR.bDeviceClass,
    .bDeviceSubClass = 0, //TALABARDINE_USB_DESCRIPTOR.bDeviceSubClass,
    .bDeviceProtocol = 0, //TALABARDINE_USB_DESCRIPTOR.bDeviceProtocol,
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

static struct
{
    usb_configuration_cb configuration_cb;
    uint16_t selected_configuration;
} context = {
    .configuration_cb = NULL,
    .selected_configuration = 0
};

static void usb_set_address_callback(const struct udc_control_callback *cb)
{
    udc_set_address(cb->wValue);
}

static void usb_set_config_callback(const struct udc_control_callback *cb)
{
    uint16_t config = cb->wValue;
    if(!config)
    {
        sercom_usart_puts(SERCOM_MIDI_CHANNEL, "Unconf");
        udc_endpoint_unconfigure();
    }
    else
    {
        const uint8_t *ptr = (uint8_t*)&TALABARDINE_CONFIG0_DESCRIPTOR;
        const uint8_t *end = ptr + TALABARDINE_CONFIG0_DESCRIPTOR.configuration.wTotalLength;
        for(ptr += ptr[0]; ptr < end; ptr += ptr[0])
            if(ptr[1] == 0x05) // Endpoint descriptor
                udc_endpoint_configure((void*)ptr);

        // Notify higher level
        if(context.configuration_cb != NULL)
            context.configuration_cb(config);
    }
    context.selected_configuration = config;
}

void usb_setup_packet(const volatile void *_buf)
{
    const volatile struct usb_setup_packet_t *packet = _buf;
    // TODO: order this
    uint16_t length = packet->wLength;
    uint8_t bRequest = packet->bRequest;
    uint8_t bmRequestType = packet->bmRequestType;
    uint16_t wValue = packet->wValue;

    // GET
    if(bmRequestType == 0x80)
    {
        uint16_t type = (wValue >> 8);
        uint16_t id   = (wValue & 0xff);
        switch(bRequest)
        {
            case 0x06: //GET_DESCRIPTOR
                switch(type)
                {
                    case 0x01: // DEVICE
                        if(id == 0)
                            udc_tx(0, &TALABARDINE_USB_DESCRIPTOR, sizeof(TALABARDINE_USB_DESCRIPTOR));
                        else
                            udc_stall(0);
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
                            udc_stall(0);
                        break;
                    case 0x03: // STRING
                        if(id == 0)
                            udc_tx(0, &TALABARDINE_LANGID_DESCRIPTOR, sizeof(TALABARDINE_LANGID_DESCRIPTOR));
                        else if(id <= 3)
                            udc_tx(0, &TALABARDINE_MANUFACTURER, sizeof(TALABARDINE_MANUFACTURER));
                        else
                            udc_stall(0);
                        break;
                    case 0x06: // DEVICE QUALIFIER
                        if(id == 0)
                            udc_tx(0, &TALABARDINE_QUALIFIER_DESCRIPTOR, sizeof(TALABARDINE_QUALIFIER_DESCRIPTOR));
                        else
                            udc_stall(0);
                        break;

                    default:
                        sercom_usart_puts(SERCOM_MIDI_CHANNEL, "GD ");
                        dump(&wValue, sizeof(wValue));
                        udc_stall(0);
                }
                break;

            default:
                sercom_usart_puts(SERCOM_MIDI_CHANNEL, "[GET] Unknown SETUP.bRequest ");
                dump(&bRequest, sizeof(bRequest));
                sercom_usart_puts(SERCOM_MIDI_CHANNEL, "\r\n");
        }
    }
    // SET
    else if(bmRequestType == 0x00)
    {
        switch(bRequest)
        {
            case 0x05: // SET_ADDRESS
            {
                struct udc_control_callback cb = {
                    .type = UDC_CONTROL_SET_ADDRESS,
                    .wValue = wValue,
                    .callback = usb_set_address_callback
                };
                udc_control_send(&cb);
                break;
            }

            case 0x09: // SET_CONFIGURATION
            {
                struct udc_control_callback cb = {
                    .type = UDC_CONTROL_SET_CONFIG,
                    .wValue = wValue,
                    .callback = usb_set_config_callback
                };
                udc_control_send(&cb);
                break;
            }

            default:
                sercom_usart_puts(SERCOM_MIDI_CHANNEL, "[SET] Unknown SETUP.bRequest ");
                dump(&bRequest, sizeof(bRequest));
                sercom_usart_puts(SERCOM_MIDI_CHANNEL, "\r\n");
        }
    }
    else
        sercom_usart_puts(SERCOM_MIDI_CHANNEL, "OOPS\r\n");
}

bool usb_is_configured(uint16_t config)
{
    return context.selected_configuration == config;
}

void usb_set_configuration_callback(usb_configuration_cb callback)
{
    context.configuration_cb = callback;
}

