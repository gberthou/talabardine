#include "usb_talabardine.h"
#include "udc.h"
#include "usb.h"
#include "utils.h"

static volatile uint8_t bulk_input[64];

static void usb_talabardine_on_set_configuration(uint16_t configIndex)
{
    (void) configIndex;
    udc_endpoint_set_buffer(2, ENDPOINT_OUT, bulk_input);
}

static void usb_talabardine_receive(void)
{
    // Data available in bulk_input
}

void usb_talabardine_init(void)
{
    udc_init();
    usb_set_configuration_callback(usb_talabardine_on_set_configuration);
    udc_set_receive_callback(2, usb_talabardine_receive);
    udc_attach();
}
