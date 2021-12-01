#ifndef UDC_H
#define UDC_H

#include <stdint.h>

#include "usb_common.h"

void udc_init(void);
void udc_attach(void);
void udc_endpoint_configure(const struct usb_endpoint_descriptor_t *descriptor);
void udc_endpoint_unconfigure(void);
void udc_tx(uint8_t ep, const void *data, size_t size);
void udc_control_send(void);
void udc_set_address(uint8_t address);
void udc_stall(void);

#endif
