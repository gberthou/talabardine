#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "udc.h"
#include "usb.h"
#include "gclk.h"
#include "nvm.h"
#include "sercom.h"
#include "config.h"

// https://github.com/ataradov/dgw/blob/master/embedded/udc.c
// https://www.beyondlogic.org/usbnutshell/usb4.shtml
// -> control buffers <= 64 bytes
#define EP_BUFFER_SIZE 64

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
#define DBG_INTFLAG_ALL 0x03fd

#define EPINTFLAG_TRCPT0  (1 << 0)
#define EPINTFLAG_TRCPT1  (1 << 1)
#define EPINTFLAG_TRFAIL0 (1 << 2)
#define EPINTFLAG_TRFAIL1 (1 << 3)
#define EPINTFLAG_RXSTP   (1 << 4)
#define EPINTFLAG_STALL0  (1 << 5)
#define EPINTFLAG_STALL1  (1 << 6)
#define DBG_EPINTFLAG_ALL 0x7f

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

static struct usb_descriptor_t __attribute__((aligned(4))) descriptors[NENDPOINTS];
static uint8_t __attribute__((aligned(4))) buf0[EP_BUFFER_SIZE];
static uint8_t __attribute__((aligned(4))) buf1[EP_BUFFER_SIZE];
static uint8_t last_address;

enum endpoint_direction_e
{
    ENDPOINT_OUT,
    ENDPOINT_IN
};

static void endpoint_reset(uint8_t ep, enum endpoint_direction_e direction)
{
    volatile struct usb_device_endpoint_register_t *endpoint = ENDPOINT(ep);
    uint8_t mask = (direction == ENDPOINT_OUT ? 0xf8 : 0x8f);
    endpoint->epcfg &= mask;
}

static void udc_reset(void)
{
    volatile struct usb_device_endpoint_register_t *endpoint0 = ENDPOINT(0);

    udc_endpoint_unconfigure();

    endpoint0->epcfg = 0x11; // Control OUT, Control OUT
    descriptors[0].banks[0].addr = (uint32_t) &buf0[0];
    descriptors[0].banks[0].pcksize = (0x3 << 28); // 64 bytes

    descriptors[0].banks[1].addr = (uint32_t) &buf1[0];
    descriptors[0].banks[1].pcksize = (0x3 << 28); // 64 bytes

    USB->dadd = (1 << 7) // ADDEN
              | last_address
              ;

    endpoint0->epintflag = EPINTFLAG_RXSTP;
    endpoint0->epstatusset = EPSTATUS_BK0RDY;
    endpoint0->epintenset = EPINTFLAG_RXSTP;
}

void udc_endpoint_configure(const struct usb_endpoint_descriptor_t *descriptor)
{
    uint8_t ep = (descriptor->bEndpointAddress & 0x7f);
    uint8_t directionIn = (descriptor->bEndpointAddress & 0x80);
    uint8_t transferType = (descriptor->bmAttributes & 0x3);
    uint16_t size = (descriptor->wMaxPacketSize & 0x03ff);
    volatile struct usb_device_endpoint_register_t *endpoint = ENDPOINT(ep);

    // Configure endpoint transfer type
    uint8_t epcfg = endpoint->epcfg;
    epcfg &= (0x07 << (directionIn ? 0 : 4));
    epcfg |= ((transferType + 1) << (directionIn ? 4 : 0));
    endpoint->epcfg = epcfg;

    // Compute driver's packet size
    size_t log2_size = 0;
    for(size >>= 3; size && log2_size <= 7; ++log2_size, size >>= 1);
    descriptors[ep].banks[directionIn ? 1 : 0].pcksize = (log2_size << 28);

    if(directionIn)
    {
        endpoint->epintenset = EPINTFLAG_TRCPT1;
        endpoint->epstatusclr = EPSTATUS_BK1RDY | EPSTATUS_DTGLIN;
    }
    else
    {
        endpoint->epintenset = EPINTFLAG_TRCPT0;
        endpoint->epstatusclr = EPSTATUS_DTGLOUT;
        endpoint->epstatusset = EPSTATUS_BK0RDY;
    }
}

void udc_endpoint_unconfigure(void)
{
    for(size_t i = 1; i < NENDPOINTS; ++i)
    {
        endpoint_reset(i, ENDPOINT_OUT);
        endpoint_reset(i, ENDPOINT_IN);
    }
}

void udc_attach(void)
{
    USB->ctrlb &= ~CTRLB_DETACH;
}

void udc_init(void)
{
    memset(descriptors, 0, sizeof(descriptors));

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
    last_address = 0;
    udc_reset();

    USB->intflag = INTFLAG_EORST | INTFLAG_WAKEUP;
#if 0
    USB->intenset = INTFLAG_EORST | INTFLAG_WAKEUP;
#else
    USB->intenset = DBG_INTFLAG_ALL;
#endif
}

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

void udc_tx(uint8_t ep, const void *data, size_t size)
{
    volatile struct usb_device_endpoint_register_t *endpoint = ENDPOINT(ep);

    uint8_t *dst = buf1;
    if(data == NULL)
        size = 0;
#if 1
    const uint8_t *src = data;
    for(size_t n = size; n--;)
        *dst++ = *src++;
#else
    // TODO: Make this work
    descriptors[ep].banks[1].addr = (uint32_t) data;
#endif
    uint32_t pcksize = (0x3 << 28) // 64 bytes per packet
                     | (size)
                     ;
#if 0
    if(size > 64)
    {
        size_t q = (size >> 6);
        size_t r = size - (q << 6);
        q += (r != 0 ? 1 : 0);
        size_t x = (q << 6);

        pcksize |= (x << 14); // MUTI_PACKET_SIZE
    }
#endif
    descriptors[ep].banks[1].pcksize = pcksize;

    endpoint->epintflag = EPINTFLAG_TRCPT1 | EPINTFLAG_TRFAIL1;
#if 0
    endpoint0->epintenset = EPINTFLAG_TRCPT1 | EPINTFLAG_TRFAIL1;
#else
    endpoint->epintenset = DBG_EPINTFLAG_ALL;
#endif
    endpoint->epstatusset = EPSTATUS_BK1RDY;
}

void udc_control_send(void)
{
    volatile struct usb_device_endpoint_register_t *endpoint0 = ENDPOINT(0);
    descriptors[0].banks[1].pcksize = (0x3 << 28)
                                    | (1 << 31) // AUTO_ZLP
                                    ;
    endpoint0->epintflag = EPINTFLAG_TRCPT1 | EPINTFLAG_TRFAIL1;
#if 0
    endpoint0->epintenset = EPINTFLAG_TRCPT1 | EPINTFLAG_TRFAIL1;
#else
    endpoint0->epintenset = DBG_EPINTFLAG_ALL;
#endif
    endpoint0->epstatusset = EPSTATUS_BK1RDY;

    while(!(endpoint0->epintflag & EPINTFLAG_TRCPT1));
}

#if 0
void usb_rx(size_t size)
{
    volatile struct usb_device_endpoint_register_t *endpoint0 = ENDPOINT(0);
    descriptors[0].banks[0].pcksize = (0x3 << 28)
                                    | (size) // MULTI_PACKET_SIZE
                                    ;

    endpoint0->epintflag = EPINTFLAG_TRCPT0 | EPINTFLAG_TRFAIL0;
#if 0
    endpoint0->epintenset = EPINTFLAG_TRCPT0;
#else
    endpoint0->epintenset = DBG_EPINTFLAG_ALL;
#endif
    //endpoint0->epstatusset = EPSTATUS_BK0RDY;
}
#endif

void udc_set_address(uint8_t address)
{
    last_address = address & 0x7f;
    USB->dadd = (1 << 7) // ADDEN
              | last_address
              ;
}

void udc_stall(void)
{
    ENDPOINT(0)->epstatusset = EPSTATUS_STALLRQ1;
}

void usb_handler(void)
{
    uint16_t intflag = USB->intflag;
    uint16_t summary = USB->epintsmry;
    
    //dump(&intflag, 2);
    //dump(&summary, 2);

    USB->intflag = INTFLAG_SOF;
    if(intflag & INTFLAG_SUSPEND)
        USB->intflag = INTFLAG_SUSPEND;
    if(intflag & INTFLAG_WAKEUP)
        USB->intflag = INTFLAG_WAKEUP;
    if(intflag & INTFLAG_EORST)
    {
        udc_reset();
        USB->intflag = INTFLAG_EORST;
        sercom_usart_puts(SERCOM_MIDI_CHANNEL, "\r\nR@"); 
        uint8_t tmp = USB->dadd;
        dump(&tmp, sizeof(tmp));
    }
    if(intflag & ~(INTFLAG_SUSPEND | INTFLAG_WAKEUP | INTFLAG_EORST | INTFLAG_SOF))
    {
        sercom_usart_puts(SERCOM_MIDI_CHANNEL, "Unsupported INTFLAG: "); 
        dump(&intflag, sizeof(intflag));
    }

    for(size_t i = 0; summary; ++i, summary >>= 1)
    {
        if(!(summary & 1))
            continue;
        if(i != 0)
        {
            uint8_t a=i;
            dump(&a, sizeof(a));
        }
        volatile struct usb_device_endpoint_register_t *endpoint = ENDPOINT(i);
        uint8_t epintflag = endpoint->epintflag;

        if(epintflag & EPINTFLAG_RXSTP)
        {
            endpoint->epintflag = EPINTFLAG_RXSTP;
            usb_setup_packet(buf0);
            endpoint->epstatusclr = EPSTATUS_BK0RDY;
        }
        if(epintflag & EPINTFLAG_TRCPT0)
        {
            endpoint->epintflag = EPINTFLAG_TRCPT0;
            endpoint->epstatusset = EPSTATUS_BK0RDY;
        }
        
        if(epintflag & EPINTFLAG_TRCPT1)
        {
            endpoint->epintflag = EPINTFLAG_TRCPT1;
            endpoint->epstatusclr = EPSTATUS_BK1RDY;
        }
        if(epintflag & EPINTFLAG_TRFAIL0)
        {
            sercom_usart_puts(SERCOM_MIDI_CHANNEL, "TRFAIL0");
            USB->ctrlb = CTRLB_DETACH;
            endpoint->epintflag = EPINTFLAG_TRFAIL0;
        }
        if(epintflag & EPINTFLAG_TRFAIL1)
        {
            sercom_usart_puts(SERCOM_MIDI_CHANNEL, "TRFAIL1");
            USB->ctrlb = CTRLB_DETACH;
            endpoint->epintflag = EPINTFLAG_TRFAIL1;
        }

        if(epintflag & ~(EPINTFLAG_RXSTP | EPINTFLAG_TRCPT0 | EPINTFLAG_TRCPT1 | EPINTFLAG_TRFAIL0 | EPINTFLAG_TRFAIL1))
        {
            sercom_usart_puts(SERCOM_MIDI_CHANNEL, "Unsupported EPINTFLAG: "); 
            dump(&epintflag, sizeof(epintflag));
        }
    }
}

