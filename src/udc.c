#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "udc.h"
#include "usb.h"
#include "gclk.h"
#include "nvm.h"
#include "sercom.h"
#include "config.h"
#include "utils.h"

// https://github.com/ataradov/dgw/blob/master/embedded/udc.c
// https://www.beyondlogic.org/usbnutshell/usb4.shtml
// -> control buffers <= 64 bytes
#define EP_BUFFER_SIZE 128

#define USB ((volatile struct usb_device_t*) 0x41005000)
#define _ENDPOINT0 (0x41005100)
#define _ENDPOINT(x) (_ENDPOINT0 + ((x) << 5))
#define ENDPOINT(x) ((volatile struct usb_device_endpoint_register_t*) _ENDPOINT(x))

#define NENDPOINTS 8

#define CTRLB_DETACH (1 << 0)

#define SYNCBUSY_SWRST  (1 << 0)
#define SYNCBUSY_ENABLE (1 << 1)

#define INTFLAG_SUSPEND (1 << 0)
#define INTFLAG_SOF (1 << 2)
#define INTFLAG_EORST (1 << 3)
#define INTFLAG_WAKEUP (1 << 4)
#define INTFLAG_EORSM (1 << 5)

#define EPINTFLAG_TRCPT0  (1 << 0)
#define EPINTFLAG_TRCPT1  (1 << 1)
#define EPINTFLAG_TRFAIL0 (1 << 2)
#define EPINTFLAG_TRFAIL1 (1 << 3)
#define EPINTFLAG_RXSTP   (1 << 4)
#define EPINTFLAG_STALL0  (1 << 5)
#define EPINTFLAG_STALL1  (1 << 6)

#define EPSTATUS_DTGLOUT (1 << 0)
#define EPSTATUS_DTGLIN (1 << 1)
#define EPSTATUS_STALLRQ1 (1 << 5)
#define EPSTATUS_BK0RDY (1 << 6)
#define EPSTATUS_BK1RDY (1 << 7)

struct __attribute__((packed)) usb_device_endpoint_register_t
{
    uint8_t epcfg;
    uint8_t _pad0[3];
    uint8_t epstatusclr;
    uint8_t epstatusset;
    uint8_t epstatus;
    uint8_t epintflag;
    uint8_t epintenclr;
    uint8_t epintenset;
    uint8_t _pad1[22];
};

struct __attribute__((packed)) usb_device_t
{
    uint8_t ctrla;
    uint8_t _pad0;
    uint8_t syncbusy;
    uint8_t qosctrl;

    uint32_t _pad1;
    uint16_t ctrlb;
    uint8_t dadd;
    uint8_t _pad2;
    uint8_t status;

    uint8_t fsmstatus;

    uint16_t _pad3;
    uint16_t fnum;
    uint16_t _pad4;
    uint16_t intenclr;
    uint16_t _pad5;
    uint16_t intenset;
    uint16_t _pad6;
    uint16_t intflag;
    uint16_t _pad7;
    uint16_t epintsmry;
    uint16_t _pad8;

    uint32_t descadd;
    uint16_t padcal;

    uint8_t _pad9[0xd6];
    struct usb_device_endpoint_register_t endpoints[NENDPOINTS];
};

struct __attribute__((packed)) usb_device_bank_t
{
    uint32_t addr;
    uint32_t pcksize;
    uint16_t extreg; // Bank 0 only, otherwise reserved
    uint8_t status_bk;
    uint8_t _pad0[5];
};

struct __attribute__((packed)) usb_descriptor_t
{
    struct usb_device_bank_t banks[2];
};

static volatile struct usb_descriptor_t __attribute__((aligned(4))) descriptors[NENDPOINTS];
static volatile uint8_t __attribute__((aligned(4))) buf0[EP_BUFFER_SIZE];
static volatile uint8_t __attribute__((aligned(4))) buf1[EP_BUFFER_SIZE];

static struct
{
    void (*receive_callbacks[NENDPOINTS-1])(void);
    bool pending_tx[NENDPOINTS-1];
    
    uint8_t last_address;
    bool suspended;
    struct udc_control_callback pending_control;
} context;

static void endpoint_reset(uint8_t ep, enum endpoint_direction_e direction)
{
    volatile struct usb_device_endpoint_register_t *endpoint = ENDPOINT(ep);
    endpoint->epcfg &= (direction == ENDPOINT_OUT ? 0xf8 : 0x8f);
    endpoint->epintenclr = (direction == ENDPOINT_OUT ? 0x35 : 0x5a);
    endpoint->epintflag = (direction == ENDPOINT_OUT ? 0x35 : 0x5a);
    endpoint->epstatusclr = (direction == ENDPOINT_OUT ? 0x55: 0xa2);
}

/*
static uint8_t busy_wait_for_ep(uint8_t ep, uint8_t epintmask)
{
    volatile struct usb_device_endpoint_register_t *endpoint = ENDPOINT(ep);
    uint8_t ret;
    endpoint->epintflag = epintmask;
    endpoint->epintenclr = epintmask;
    while(!(ret = (endpoint->epintflag & epintmask)));
    
    endpoint->epintflag = ret;
    return ret;
}
*/

static void async_wait_for_ep(uint8_t ep, uint8_t epintmask)
{
    volatile struct usb_device_endpoint_register_t *endpoint = ENDPOINT(ep);
    endpoint->epintflag = epintmask;
    endpoint->epintenset = epintmask;
}

static void udc_reset(void)
{
    volatile struct usb_device_endpoint_register_t *endpoint0 = ENDPOINT(0);

    udc_endpoint_unconfigure();
    
    context.suspended = false;
    context.pending_control.type = UDC_CONTROL_NONE;

    endpoint0->epcfg = 0x11; // Control OUT, Control OUT
    descriptors[0].banks[0].addr = (uint32_t) &buf0[0];
    descriptors[0].banks[0].pcksize = (0x3 << 28); // 64 bytes

    descriptors[0].banks[1].addr = (uint32_t) &buf1[0];
    descriptors[0].banks[1].pcksize = (0x3 << 28); // 64 bytes

    USB->dadd = (1 << 7) // ADDEN
              | context.last_address
              ;

    endpoint0->epstatusclr = EPSTATUS_DTGLOUT | EPSTATUS_DTGLIN | EPSTATUS_BK0RDY |EPSTATUS_BK1RDY;
    async_wait_for_ep(0, EPINTFLAG_RXSTP);
}

void udc_endpoint_configure(const struct usb_endpoint_descriptor_t *descriptor)
{
    uint8_t ep = (descriptor->bEndpointAddress & 0x7f);
    uint8_t directionIn = (descriptor->bEndpointAddress & 0x80);
    uint8_t transferType = (descriptor->bmAttributes & 0x3);
    volatile struct usb_device_endpoint_register_t *endpoint = ENDPOINT(ep);

    // wMaxPacketSize might be misaligned
    const uint8_t *psize = (const uint8_t*) &descriptor->wMaxPacketSize;
    uint16_t size_l = psize[0];
    uint16_t size_h = (psize[1] & 0x03);
    uint16_t size = size_l | (size_h << 8);
    
    endpoint_reset(ep, directionIn);

    // Configure endpoint transfer type
    uint8_t epcfg = endpoint->epcfg;
    epcfg &= (0x07 << (directionIn ? 0 : 4));
    epcfg |= ((transferType + 1) << (directionIn ? 4 : 0));
    endpoint->epcfg = epcfg;

    if(!directionIn)
    {
        // Compute driver's packet size
        size_t log2_size = 0;
        for(size_t s = (size >> 3); s && log2_size <= 7; ++log2_size, s >>= 1);
        --log2_size;
        uint32_t pcksize = (log2_size << 28);
        
        descriptors[ep].banks[0].pcksize = pcksize | size;
        endpoint->epstatusclr = EPSTATUS_BK0RDY;
        async_wait_for_ep(ep, EPINTFLAG_TRCPT0 | EPINTFLAG_TRFAIL0);
    }
    // If direction IN, do nothing
}

void udc_dump_endpoint(uint8_t ep)
{
    volatile struct usb_device_endpoint_register_t *endpoint = ENDPOINT(ep);
    sercom_usart_puts(SERCOM_MIDI_CHANNEL, "\r\nFSMSTATE: ");
    dump((void*)&USB->fsmstatus, 1);
    sercom_usart_puts(SERCOM_MIDI_CHANNEL, "\r\nEndpoint ");
    dump(&ep, 1);
    sercom_usart_puts(SERCOM_MIDI_CHANNEL, ":\r\n");
    dump((void*)endpoint, 10);
    sercom_usart_puts(SERCOM_MIDI_CHANNEL, "\r\nDesc:\r\n");
    dump((void*)&descriptors[ep], sizeof(descriptors[0]));
    sercom_usart_puts(SERCOM_MIDI_CHANNEL, "\r\n");
}

void udc_endpoint_unconfigure(void)
{
    for(size_t i = 1; i < NENDPOINTS; ++i)
    {
        endpoint_reset(i, ENDPOINT_OUT);
        endpoint_reset(i, ENDPOINT_IN);
    }
}

void udc_endpoint_set_buffer(uint8_t ep, enum endpoint_direction_e direction, volatile void *buffer)
{
    descriptors[ep].banks[direction].addr = (uint32_t) buffer;
}

void udc_attach(void)
{
    USB->ctrlb &= ~CTRLB_DETACH;
}

/*
static void udc_detach(void)
{
    USB->ctrlb |= CTRLB_DETACH;
}
*/

bool udc_is_attached(void)
{
    return (USB->ctrlb & CTRLB_DETACH) == 0;
}

void udc_init(void)
{
#if 0
    memset((void*)descriptors, 0, sizeof(descriptors));
#else
    volatile uint8_t *end = ((uint8_t*)descriptors) + sizeof(descriptors);
    for(volatile uint8_t *ptr = (uint8_t*)descriptors; ptr < end; *ptr++ = 0);
#endif

    // 48MHz
    gclk_connect_clock(GCLK_DST_USB, 0);

    while(USB->syncbusy & SYNCBUSY_SWRST);
    USB->ctrla = SYNCBUSY_SWRST;
    while(USB->syncbusy & SYNCBUSY_SWRST);

    while(USB->syncbusy & SYNCBUSY_ENABLE);
    USB->ctrla = (1 << 1) // ENABLE
               | (0 << 7) // MODE (Device)
               ;
    while(USB->syncbusy & SYNCBUSY_ENABLE);

    // Calibration
    uint32_t usbtransn = nvm_get_calibration_bits(NVM_CAL_USBTRANSN);
    uint32_t usbtransp = nvm_get_calibration_bits(NVM_CAL_USBTRANSP);
    uint32_t usbtrim   = nvm_get_calibration_bits(NVM_CAL_USBTRIM);
    USB->padcal = usbtransp
                | (usbtransn << 6)
                | (usbtrim << 12)
                ;
    USB->descadd = (uint32_t) descriptors;
    USB->dadd = 0;
    context.last_address = 0;
    udc_reset();

    USB->intflag = INTFLAG_EORST;
    USB->intenset = INTFLAG_EORST;
}

void udc_tx(uint8_t ep, const void *data, size_t size)
{
    volatile struct usb_device_endpoint_register_t *endpoint = ENDPOINT(ep);
    volatile struct usb_device_bank_t *bank = &descriptors[ep].banks[1];

    if(data == NULL)
        size = 0;
    else if(data >= (const void*) 0x20000000 && data < (const void*) 0x20004000 && (((uint32_t) data) & 0x3) == 0)
        bank->addr = (uint32_t) data;
    else
#if 0
    {
        volatile void *dst = buf1;
        memcpy(dst, data, size);
    }
#else
    {
        volatile uint8_t *dst = buf1;
        const uint8_t *ptr = data;
        size_t s = size;
        while(s--)
            *dst++ = *ptr++;
        bank->addr = (uint32_t) buf1;
    }
#endif
    uint32_t pcksize = bank->pcksize;
    pcksize &= 0xffffc000;
    if(size == 0)
        pcksize |= (1 << 31);
    else
        pcksize |= size;
    bank->pcksize = pcksize;
    
    if(ep == 0)
        async_wait_for_ep(ep, EPINTFLAG_TRCPT1 | EPINTFLAG_TRFAIL1);
    else
    {
        context.pending_tx[ep-1] = true;
        async_wait_for_ep(ep, EPINTFLAG_TRCPT1);
    }
    endpoint->epstatusset = EPSTATUS_BK1RDY;
}

void udc_busy_wait_tx_free(uint8_t ep)
{
    if(ep)
    {
        volatile bool *ptr = &context.pending_tx[ep - 1];
        while(*ptr);
    }
}

void udc_control_send(const struct udc_control_callback *cb)
{
    volatile struct usb_device_endpoint_register_t *endpoint0 = ENDPOINT(0);
    context.pending_control = *cb;
    descriptors[0].banks[1].pcksize = (0x3 << 28)
                                    | (1 << 31) // AUTO_ZLP
                                    ;
    endpoint0->epstatusset = EPSTATUS_BK1RDY;
    async_wait_for_ep(0, EPINTFLAG_TRCPT1 | EPINTFLAG_TRFAIL1);
}

/*
void usb_rx(uint8_t ep, size_t size)
{
    volatile struct usb_device_endpoint_register_t *endpoint = ENDPOINT(ep);
    descriptors[ep].banks[0].pcksize = (0x3 << 28)
                                    | (size)
                                    ;

    endpoint->epstatusset = EPSTATUS_BK0RDY;
    endpoint->epstatusclr = EPSTATUS_DTGLOUT;
    busy_wait_for_ep(0, EPINTFLAG_TRCPT0 | EPINTFLAG_TRFAIL0);
}
*/

void udc_set_address(uint8_t address)
{
    context.last_address = address & 0x7f;
    USB->dadd = (1 << 7) // ADDEN
              | context.last_address
              ;
}

void udc_stall(uint8_t ep)
{
    sercom_usart_puts(SERCOM_MIDI_CHANNEL, "Stalling...\r\n"); 
    ENDPOINT(ep)->epstatusset = EPSTATUS_STALLRQ1;
}

void udc_set_receive_callback(uint8_t ep, void (*callback)(void))
{
    if(ep < 1)
        return;
    context.receive_callbacks[ep - 1] = callback;
}

bool udc_is_suspended(void)
{
    return context.suspended;
}

void usb_handler(void)
{
    uint16_t intflag = USB->intflag;
    uint16_t summary = USB->epintsmry;
    
    if(intflag & INTFLAG_SUSPEND)
    {
        context.suspended = true;
        USB->intflag = INTFLAG_SUSPEND;
    }
    if(intflag & (INTFLAG_WAKEUP | INTFLAG_EORSM))
    {
        context.suspended = false;
        USB->intflag = INTFLAG_WAKEUP | INTFLAG_EORSM;
    }
    if(intflag & INTFLAG_EORST)
    {
        udc_reset();
        USB->intflag = INTFLAG_EORST;
    }
    if(intflag & ~(INTFLAG_SUSPEND | INTFLAG_WAKEUP | INTFLAG_EORST | INTFLAG_SOF | INTFLAG_EORSM))
    {
        sercom_usart_puts(SERCOM_MIDI_CHANNEL, "Unsupported INTFLAG: "); 
        dump(&intflag, sizeof(intflag));
    }

    for(size_t i = 0; summary; ++i, summary >>= 1)
    {
        if(!(summary & 1))
            continue;
        volatile struct usb_device_endpoint_register_t *endpoint = ENDPOINT(i);
        uint8_t epintflag = endpoint->epintflag;

        if(i == 0)
        {
            if(epintflag & EPINTFLAG_RXSTP)
            {
                usb_setup_packet(buf0);
                endpoint->epstatusclr = EPSTATUS_BK0RDY;
                endpoint->epintflag = EPINTFLAG_RXSTP;
            }
            if(epintflag & EPINTFLAG_TRCPT1)
            {
                if(context.pending_control.type != UDC_CONTROL_NONE)
                    context.pending_control.callback(&context.pending_control);
                context.pending_control.type = UDC_CONTROL_NONE;
                endpoint->epstatusclr = EPSTATUS_BK1RDY;
                endpoint->epintflag = EPINTFLAG_TRCPT1;
            }
        }
        else
        {
            if(epintflag & EPINTFLAG_TRCPT0)
            {
                void (*f)(void) = context.receive_callbacks[i-1];
                if(f)
                    f();
            
                endpoint->epstatusclr = EPSTATUS_BK0RDY;
                endpoint->epintflag = EPINTFLAG_TRCPT0;
            }
        
            if(epintflag & EPINTFLAG_TRCPT1)
            {
                context.pending_tx[i-1] = false;
                endpoint->epintflag = EPINTFLAG_TRCPT1;
            }
            if(epintflag & EPINTFLAG_TRFAIL0)
            {
                endpoint->epintflag = EPINTFLAG_TRFAIL0;
            }
            if(epintflag & EPINTFLAG_STALL1)
            {
                udc_dump_endpoint(i);
                endpoint->epstatusset = EPSTATUS_STALLRQ1;
                endpoint->epintflag = EPINTFLAG_STALL1;
            }
            if(epintflag & EPINTFLAG_TRFAIL1)
            {
                endpoint->epstatusclr = EPSTATUS_BK1RDY;
                endpoint->epintflag = EPINTFLAG_TRFAIL1;
            }

            if(epintflag & ~(EPINTFLAG_TRCPT0 | EPINTFLAG_TRCPT1 | EPINTFLAG_STALL1 |EPINTFLAG_TRFAIL0 | EPINTFLAG_TRFAIL1))
            {
                sercom_usart_puts(SERCOM_MIDI_CHANNEL, "Unsupported EPINTFLAG: "); 
                dump(&epintflag, sizeof(epintflag));
            }
        }
    }
}

