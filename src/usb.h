#ifndef USB_H
#define USB_H

#include <stdint.h>
#include <stdbool.h>

typedef void (*usb_configuration_cb)(uint16_t);

void usb_setup_packet(const volatile void *_buf);
bool usb_is_configured(uint16_t config);
void usb_set_configuration_callback(usb_configuration_cb callback);

#endif

