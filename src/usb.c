#include <stdint.h>
#include <stddef.h>

#include "usb.h"
#include "udc.h"
#include "usb_common.h"
#include "sercom.h"
#include "config.h"
#include "utils.h"

#define USB_MAX_CONFIGURATION_AMOUNT 4
#define USB_MAX_STRING_AMOUNT 16
#define USB_MAX_QUALIFIER_AMOUNT 4

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

static struct
{
    const struct usb_device_descriptor_t *device_descriptor;
    const struct usb_configuration_descriptor_t *configuration_descriptors[USB_MAX_CONFIGURATION_AMOUNT];
    const void *string_descriptors[USB_MAX_STRING_AMOUNT];
    const struct usb_qualifier_descriptor_t *qualifier_descriptors[USB_MAX_QUALIFIER_AMOUNT];
    
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
    if(!config || config >= USB_MAX_CONFIGURATION_AMOUNT + 1)
        udc_endpoint_unconfigure();
    else
    {
        const struct usb_configuration_descriptor_t *descriptor = context.configuration_descriptors[config - 1];
        if(!descriptor)
            udc_endpoint_unconfigure();
        else
        {
            const uint8_t *ptr = (const uint8_t*) descriptor;
            const uint8_t *end = ptr + descriptor->wTotalLength;
            for(ptr += ptr[0]; ptr < end; ptr += ptr[0])
                if(ptr[1] == 0x05) // Endpoint descriptor
                    udc_endpoint_configure((void*)ptr);

            // Notify higher level
            if(context.configuration_cb != NULL)
                context.configuration_cb(config);
        }
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
                            udc_tx(0, context.device_descriptor, sizeof(*context.device_descriptor));
                        else
                            udc_stall(0);
                        break;
                    case 0x02: // CONFIGURATION
                        if(id < USB_MAX_CONFIGURATION_AMOUNT)
                        {
                            const struct usb_configuration_descriptor_t *descriptor = context.configuration_descriptors[id];
                            if(descriptor)
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
                                size_t size = (length <= descriptor->wTotalLength ? length : descriptor->wTotalLength);
                                udc_tx(0, descriptor, size);
                                break;
                            }
                        }
                        udc_stall(0);
                        break;
                    case 0x03: // STRING
                        if(id < USB_MAX_STRING_AMOUNT)
                        {
                            const void *descriptor = context.string_descriptors[id];
                            if(descriptor)
                            {
                                uint8_t size = *(const uint8_t*) descriptor;
                                udc_tx(0, descriptor, size);
                                break;
                            }
                        }
                        udc_stall(0);
                        break;
                    case 0x06: // DEVICE QUALIFIER
                        if(id < USB_MAX_QUALIFIER_AMOUNT)
                        {
                            const struct usb_qualifier_descriptor_t *descriptor = context.qualifier_descriptors[id];
                            if(descriptor)
                            {
                                udc_tx(0, descriptor, sizeof(*descriptor));
                                break;
                            }
                        }
                        udc_stall(0);
                        break;

                    default:
                        // Unsupported GET_DESCRIPTOR
                        udc_stall(0);
                }
                break;

            default:
                // Unsupported SETUP.bRequest
                break;
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
                // Unsupported SETUP.bRequest
                break;
        }
    }
    else
    {
        // Unsupported bmRequestType
    }
}

bool usb_is_configured(uint16_t config)
{
    return context.selected_configuration == config;
}

void usb_set_configuration_callback(usb_configuration_cb callback)
{
    context.configuration_cb = callback;
}

void usb_set_device_descriptor(const struct usb_device_descriptor_t *descriptor)
{
    context.device_descriptor = descriptor;
}

bool usb_set_configuration_descriptor(uint16_t config, const void *descriptor)
{
    if(config == 0 || config >= USB_MAX_CONFIGURATION_AMOUNT + 1)
        return false;
    context.configuration_descriptors[config - 1] = descriptor;
    return true;
}

bool usb_set_string_descriptor(uint16_t id, const void *descriptor)
{
    if(id >= USB_MAX_STRING_AMOUNT)
        return false;
    context.string_descriptors[id] = descriptor;
    return true;
}

bool usb_set_qualifier_descriptor(uint16_t id, const struct usb_qualifier_descriptor_t *descriptor)
{
    if(id >= USB_MAX_QUALIFIER_AMOUNT)
        return false;
    context.qualifier_descriptors[id] = descriptor;
    return true;
}