#include "usb_talabardine.h"
#include "udc.h"
#include "usb.h"
#include "usb_midi.h"
#include "utils.h"

const struct usb_device_descriptor_t __attribute__((aligned(4))) TALABARDINE_DEVICE_DESCRIPTOR = {
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

    } TALABARDINE_CONFIG1_DESCRIPTOR = {
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
    .bLength = sizeof(TALABARDINE_LANGID_DESCRIPTOR),
    .bDescriptorType = 0x03, // String Descriptor
    .wLANGID[0] = 0x0409 // en_US
};
const USB_UNICODE_DESCRIPTOR_T(TALABARDINE_MANUFACTURER, u"gberthou");
const USB_UNICODE_DESCRIPTOR_T(TALABARDINE_PRODUCT, u"Talabardine");
const USB_UNICODE_DESCRIPTOR_T(TALABARDINE_SERIAL, u"0000-0000");

static volatile uint8_t bulk_input[64];

static void usb_talabardine_on_set_configuration(uint16_t configIndex)
{
    (void) configIndex;
    udc_endpoint_set_buffer(2, ENDPOINT_OUT, bulk_input);
}

void usb_talabardine_init(void)
{
    udc_init();
    usb_set_configuration_callback(usb_talabardine_on_set_configuration);
    
    usb_set_device_descriptor(&TALABARDINE_DEVICE_DESCRIPTOR);
    
    usb_set_configuration_descriptor(1, &TALABARDINE_CONFIG1_DESCRIPTOR);
    
    usb_set_string_descriptor(0, &TALABARDINE_LANGID_DESCRIPTOR);
    usb_set_string_descriptor(1, &TALABARDINE_MANUFACTURER);
    usb_set_string_descriptor(2, &TALABARDINE_PRODUCT);
    usb_set_string_descriptor(3, &TALABARDINE_SERIAL);
        
    usb_set_qualifier_descriptor(0, &TALABARDINE_QUALIFIER_DESCRIPTOR);
    udc_attach();
}
