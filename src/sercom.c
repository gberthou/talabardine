#include <stdint.h>

#include "sercom.h"
#include "gclk.h"
#include "config.h"

struct __attribute__((packed)) sercom_usart_t
{
    uint32_t ctrla;
    uint32_t ctrlb;
    uint32_t _pad0;
    uint16_t baud;
    uint8_t rxpl;
    uint8_t _pad1[5];
    uint8_t intenclr;
    uint8_t _pad2;
    uint8_t intenset;
    uint8_t _pad3;
    uint8_t intflag;
    uint8_t _pad4;
    uint16_t status;
    uint32_t syncbusy;
    uint32_t _pad5[2];
    uint16_t data;
    uint16_t _pad6[3];
    uint8_t dbgctrl;
    uint8_t _pad7[3];
    uint16_t fifospace;
    uint16_t fifoptr;
};

struct __attribute__((packed)) sercom_spi_t
{
    uint32_t ctrla;
    uint32_t ctrlb;
    uint32_t _pad0;
    uint8_t baud;
    uint8_t _pad1[7];
    uint8_t intenclr;
    uint8_t _pad2;
    uint8_t intenset;
    uint8_t _pad3;
    uint8_t intflag;
    uint8_t _pad4;
    uint16_t status;
    uint32_t syncbusy;
    uint32_t _pad5;
    uint32_t addr;
    uint16_t data;
    uint16_t _pad6[3];
    uint8_t dbgctrl;
    uint8_t _pad7[3];
    uint16_t fifospace;
    uint16_t fifoptr;
};

struct __attribute__((packed)) sercom_i2c_master_t
{
    uint32_t ctrla;
    uint32_t ctrlb;
    uint32_t _pad0;
    uint32_t baud;
    uint32_t _pad1;
    uint8_t intenclr;
    uint8_t _pad2;
    uint8_t intenset;
    uint8_t _pad3;
    uint8_t intflag;
    uint8_t _pad4;
    uint16_t status;
    uint32_t syncbusy;
    uint32_t _pad5;
    uint32_t addr;
    uint16_t data;
    uint16_t _pad6[3];
    uint8_t dbgctrl;
    uint8_t _pad7[3];
    uint16_t fifospace;
    uint16_t fifoptr;
};

#define SERCOM0 0x42000800
#define SERCOM(i) ((volatile void*)(SERCOM0 + ((i) << 10)))

#define SYNCBUSY_SWRST (1 << 0)
#define SYNCBUSY_ENABLE (1 << 1)
#define SYNCBUSY_SYSOP  (1 << 2)

#define INTFLAG_DRE (1 << 0)
#define INTFLAG_RXC (1 << 2)

#define I2C_CTRLA_SWRST (1 << 0)
#define I2C_CTRLA_ENABLE (1 << 1)
#define I2C_CTRLA_MODE_MASTER (0x5 << 2)
#define I2C_CTRLA_SCLSM (1 << 27)

#define I2C_CTRLB_CMD_NOP (0x0 << 16)
#define I2C_CTRLB_CMD_ACKSTART (0x1 << 16)
#define I2C_CTRLB_CMD_ACKREAD (0x2 << 16)
#define I2C_CTRLB_CMD_ACKSTOP (0x3 << 16)
#define I2C_CTRLB_ACKACT (1 << 18)
#define I2C_CTRLB_FIFOCLR_TX (1 << 22)
#define I2C_CTRLB_FIFOCLR_RX (1 << 23)

#define I2C_STATUS_BUSSTATE_SHIFT 4
#define I2C_STATUS_ARBLOST (1 << 1)
#define I2C_STATUS_RXNACK (1 << 2)

#define I2C_INTFLAG_MB (1 << 0)
#define I2C_INTFLAG_SB (1 << 1)
#define I2C_INTFLAG_ERROR (1 << 7)

#define NCHANS 6

#define BAUDMAX 0xffffu

enum i2c_master_busstate_e
{
    I2C_STATUS_BUSSTATE_UNKNOWN,
    I2C_STATUS_BUSSTATE_IDLE,
    I2C_STATUS_BUSSTATE_OWNER,
    I2C_STATUS_BUSSTATE_BUSY
};

static volatile struct  
{
    enum
    {
        I2C_STATE_IDLE,
        I2C_STATE_ERROR,
        I2C_STATE_WRITE,
        I2C_STATE_READ,
        I2C_STATE_DONE
    } state;
    
    union
    {
        uint8_t *ptr;
        const uint8_t *cptr;
    } data;
    
    size_t size;
} i2c_master_context[N_SERCOMS];

static char digit2char(uint8_t x)
{
    x &= 0xf;
    if(x < 10)
        return '0' + x;
    return 'a' + x - 10;
}

// Assumes a 48MHz clock
static inline uint16_t hz_to_baud_async(uint32_t f_Hz)
{
    /*
     * BAUD = (1<<16) - (1<<16) * f_Hz / 3e6  (assumes a 48MHz clock)
     * (1<<16) / 3e6       \approx    9 / 412
     * (9 / 412) * f_Hz    \approx    ((9 * f_Hz) >> 2) / 103
     *
     * Let fp = ((9 * f_Hz) >> 2),    (9 / 412) * f_Hz = fp / 103
     * (fp >> 7) < fp / 103 < (fp >> 6)
     *
     * 1. Compute fp
     * 2. Using dichotomy between (fp >> 7) and (fp >> 6), find a = (fp / 103)
     * 3. Return BAUDMAX - a
     */

    // <<1 is safe here for f_Hz is 32bit wide and f_Hz should be <= 3e6
    uint32_t fp = (f_Hz << 1) + (f_Hz >> 2);
    uint16_t b = (fp >> 6);
    uint16_t a = (b >> 1);
    while(b > a + 1)
    {
        uint32_t c = a + b;
        c >>= 1;

        uint32_t d = 103 * c;
        if(d == fp)
            return BAUDMAX - c;
        if(d > fp)
            b = c;
        else
            a = c;
    }
    return BAUDMAX - a;
}

// Assumes a 48MHz clock
static inline uint8_t hz_to_baud_sync(uint32_t f_Hz)
{
    /*
     * BAUD = 16e6 / f_Hz - 1
     */
    return 16000000u / f_Hz - 1;
}

// Assumes a 48MHz clock
static inline uint32_t hz_to_baud_i2c_master(uint32_t f_Hz)
{
    /*
     * 28.6.2.4.1
     * Let's assume BAUDLOW = 0
     * Then, BAUD = 24e6 / f_Hz - 5.36
     * Provided that t_Rise <= 15ns as specified
     */
    uint32_t baud = 24000000u / f_Hz - 5;
    if(baud > 0xff)
        baud = 0xff;
    return baud;
}


// Assumes that GCLK0 is running at 48MHz
void sercom_init_usart(uint8_t channel, enum usart_txpo_e txpo, enum usart_rxpo_e rxpo, uint32_t baudrate_Hz)
{
    channel %= NCHANS;
    volatile struct sercom_usart_t *usart = SERCOM(channel);

    // Connect SERCOMx_CORE clock to GCLK0 (main)
    // GCLK0 must be running at 48MHz prior to this point
    gclk_connect_clock(GCLK_DST_SERCOM0_CORE + channel, 0); 

    // 26.5.3: Setup CLK_SERCOMx_APB and GCLK_SERCOMx_CORE
    
    usart->ctrla = 0; // Disable
    while(usart->syncbusy & SYNCBUSY_ENABLE);

    // TODO: partity, etc.

    usart->ctrla = (0x1 << 2) // MODE (internal clock)
                 | (0x0 << 13) // SAMPR (don't use fractional baudrate)
                 | (txpo != USART_TXPO_DISABLE ? (txpo << 16) : 0) // TXPO
                 | (rxpo != USART_RXPO_DISABLE ? (rxpo << 20) : 0) // RXPO
                 | (0 << 28) // CMODE (asynchronous = UART)
                 | (1 << 30) // DORD (LSB first)
                 ;
    usart->ctrlb = (txpo != USART_TXPO_DISABLE ? (1 << 16) : 0) // TXEN
                 | (rxpo != USART_RXPO_DISABLE ? (1 << 17) : 0) // RXEN
                 | (0x3 << 22) // Clear fifos
                 ;
    while(usart->syncbusy & SYNCBUSY_SYSOP);
    usart->baud = hz_to_baud_async(baudrate_Hz);

    usart->ctrla |= (1 << 1); // Enable
    while(usart->syncbusy & SYNCBUSY_ENABLE);
}

static inline void unsafe_usart_putc(volatile struct sercom_usart_t *usart, char c)
{
    while(!(usart->intflag & INTFLAG_DRE));
    usart->data = c;
}

void sercom_usart_putc(uint8_t channel, char c)
{
    // Should be super slow :(
    channel %= NCHANS;
    volatile struct sercom_usart_t *usart = SERCOM(channel);

    unsafe_usart_putc(usart, c);
}

void sercom_usart_puts(uint8_t channel, const char *s)
{
    // Should be super slow :(
    channel %= NCHANS;
    volatile struct sercom_usart_t *usart = SERCOM(channel);

    char c;
    while((c = *s++))
        unsafe_usart_putc(usart, c);
}

void sercom_usart_display_half(uint16_t x)
{
    char buf[] = {0, 0, 0, 0, '\r', '\n', 0};

    for(size_t i = 4; i--; x >>= 4)
        buf[i] = digit2char(x);
    sercom_usart_puts(1, buf);
}

// Assumes that GCLK0 is running at 48MHz
void sercom_init_spi_master(uint8_t channel, enum spi_dopo_e dopo, enum spi_dipo_e dipo, uint32_t baudrate_Hz)
{
    channel %= NCHANS;
    volatile struct sercom_spi_t *spi = SERCOM(channel);

    // Connect SERCOMx_CORE clock to GCLK0 (main)
    // GCLK0 must be running at 48MHz prior to this point
    gclk_connect_clock(GCLK_DST_SERCOM0_CORE + channel, 0); 

    spi->ctrla = 0; // Disable
    while(spi->syncbusy & SYNCBUSY_ENABLE);

    spi->ctrla = (0x3 << 2) // MODE (SPI master)
               | (dopo << 16)
               | (dipo << 20)
               | (0x0 << 24) // FORM (SPI frame)
               | (0 << 28) // CPHA
               | (0 << 29) // CPOL
               | (0 << 30) // DORD
               ;
    spi->ctrlb = (0x0 << 0) // CHSIZE (8bit)
               | (0 << 13) // MSSEN (Software-driven /CE)
               | (1 << 17) // RXEN
               | (0x3 << 22) // Clear fifos
               ;
    while(spi->syncbusy & SYNCBUSY_SYSOP);
    spi->baud = hz_to_baud_sync(baudrate_Hz);

    spi->ctrla |= (1 << 1); // Enable
    while(spi->syncbusy & SYNCBUSY_ENABLE);
}

void sercom_spi_burst(uint8_t channel, enum gpio_port_e ce_port, uint8_t ce_pin, void *_dst, const void *_src, size_t size)
{
    channel %= NCHANS;
    volatile struct sercom_spi_t *spi = SERCOM(channel);
    uint8_t *dst = _dst;
    const uint8_t *src = _src;

    gpio_set_output(ce_port, ce_pin, false);

    while(size--)
    {
        while(!(spi->intflag & INTFLAG_DRE));
        spi->data = *src++;
        while(!(spi->intflag & INTFLAG_RXC));
        *dst++ = spi->data;
    }

    gpio_set_output(ce_port, ce_pin, true);
}

static inline enum i2c_master_busstate_e sercom_i2c_get_busstate(uint8_t channel)
{
    volatile struct sercom_i2c_master_t *i2c = SERCOM(channel);
    return ((i2c->status >> I2C_STATUS_BUSSTATE_SHIFT) & 0x3);
}

// Assumes that GCLK0 is running at 48MHz
void sercom_init_i2c_master(uint8_t channel, uint32_t baudrate_Hz)
{
    channel %= NCHANS;
    volatile struct sercom_i2c_master_t *i2c = SERCOM(channel);
    
    i2c_master_context[channel].state = I2C_STATE_IDLE;

    // Connect SERCOMx_CORE clock to GCLK0 (main)
    // GCLK0 must be running at 48MHz prior to this point
    gclk_connect_clock(GCLK_DST_SERCOMx_SLOW, 0);
    gclk_connect_clock(GCLK_DST_SERCOM0_CORE + channel, 0);

    do 
    {
        i2c->ctrla = I2C_CTRLA_SWRST; // Disable + software reset
        while(i2c->syncbusy & SYNCBUSY_SWRST);

        /*
         * 5.2. Write the Baud Rate register (BAUD) to generate the desired baud rate.
         */
        uint32_t speed;
        if(baudrate_Hz >= 1000000)
            speed = (2 << 24); // Hs-mode
        else if(baudrate_Hz > 400000)
            speed = (1 << 24); // Fm+
        else
            speed = 0; // Sm or Fm

        i2c->ctrla = I2C_CTRLA_MODE_MASTER
                   | speed
                   ;
        i2c->ctrlb = I2C_CTRLB_FIFOCLR_TX
                   | I2C_CTRLB_FIFOCLR_RX
                   ;
        while(i2c->syncbusy & SYNCBUSY_SYSOP);
        i2c->baud = hz_to_baud_i2c_master(baudrate_Hz);
                 
        i2c->ctrla |= I2C_CTRLA_ENABLE; // Enable
        while(i2c->syncbusy & SYNCBUSY_ENABLE);
        
        // Force idle mode
        i2c->status = (I2C_STATUS_BUSSTATE_IDLE << I2C_STATUS_BUSSTATE_SHIFT);
        while(i2c->syncbusy & SYNCBUSY_SYSOP);
    } while(sercom_i2c_get_busstate(channel) != I2C_STATUS_BUSSTATE_IDLE);
}

void sercom_i2c_write(uint8_t channel, uint16_t address, const uint8_t *data, size_t length)
{
    channel %= NCHANS;
    address &= 0x7f;
    address <<= 1;
    volatile struct sercom_i2c_master_t *i2c = SERCOM(channel);
    
    do 
    {
        i2c_master_context[channel].state = I2C_STATE_WRITE;
        i2c_master_context[channel].data.cptr = data;
        i2c_master_context[channel].size = length;
        
        i2c->intflag = I2C_INTFLAG_MB
                     | I2C_INTFLAG_SB
                     | I2C_INTFLAG_ERROR
                     ;
        i2c->intenset = I2C_INTFLAG_MB
                      | I2C_INTFLAG_SB
                      | I2C_INTFLAG_ERROR
                      ;
        i2c->addr = address;
        
        while(i2c_master_context[channel].state != I2C_STATE_DONE && i2c_master_context[channel].state != I2C_STATE_ERROR);
    } while(i2c_master_context[channel].state == I2C_STATE_ERROR);
    
    while(sercom_i2c_get_busstate(channel) != I2C_STATUS_BUSSTATE_IDLE);
}

void sercom_i2c_read(uint8_t channel, uint16_t address, uint8_t *data, size_t length)
{
    channel %= NCHANS;
    address &= 0x7f;
    address <<= 1;
    address |= 1;
    volatile struct sercom_i2c_master_t *i2c = SERCOM(channel);
    
    do 
    {
        i2c_master_context[channel].state = I2C_STATE_READ;
        i2c_master_context[channel].data.ptr = data;
        i2c_master_context[channel].size = length;

        i2c->intflag = I2C_INTFLAG_MB
                     | I2C_INTFLAG_SB
                     | I2C_INTFLAG_ERROR
                     ;
        i2c->intenset = I2C_INTFLAG_MB
                      | I2C_INTFLAG_SB
                      | I2C_INTFLAG_ERROR
                      ;
        i2c->addr = address;
        
        while(i2c_master_context[channel].state != I2C_STATE_DONE && i2c_master_context[channel].state != I2C_STATE_ERROR);
    } while(i2c_master_context[channel].state == I2C_STATE_ERROR);
    
    while(sercom_i2c_get_busstate(channel) != I2C_STATUS_BUSSTATE_IDLE);
}

void sercom_i2c_interrupt(uint8_t channel)
{
    volatile struct sercom_i2c_master_t *i2c = SERCOM(channel);
    uint8_t intflag = i2c->intflag;
    uint16_t status = i2c->status;
    enum i2c_master_busstate_e state = sercom_i2c_get_busstate(channel);
    
    if(status & I2C_STATUS_ARBLOST) // Case 1: Arbitration lost or bus error during address packet transmission
    {
        // Never occurred yet, thus untested
        i2c_master_context[channel].state = I2C_STATE_ERROR;
        i2c->ctrlb = I2C_CTRLB_CMD_ACKSTOP;
        i2c->status = I2C_STATUS_ARBLOST;
        i2c->intflag = I2C_INTFLAG_MB;
        return;
    }
    if(status & I2C_STATUS_RXNACK) // Case 2: Address packet transmit complete – No ACK received
    {
        i2c_master_context[channel].state = I2C_STATE_ERROR;
        i2c->ctrlb = I2C_CTRLB_CMD_ACKSTOP;
        i2c->status = I2C_STATUS_RXNACK;
        i2c->intflag = I2C_INTFLAG_MB;
        return;
    }
    if(status & 0x0747) // Not supported yet
    {
        for(;;);
    }
    
    if(state == I2C_STATUS_BUSSTATE_OWNER)
    {
        if(intflag & I2C_INTFLAG_MB) // Case 3: Address packet transmit complete – Write packet, Master on Bus set
        {
            switch(i2c_master_context[channel].state)
            {
                case I2C_STATE_WRITE:
                    if(i2c_master_context[channel].size--)
                        i2c->data = *i2c_master_context[channel].data.cptr++;
                    else
                    {
                        i2c->ctrlb = I2C_CTRLB_CMD_ACKSTOP;
                        while(i2c->syncbusy & SYNCBUSY_SYSOP);
                        i2c_master_context[channel].state = I2C_STATE_DONE;
                    }
                    i2c->intflag = I2C_INTFLAG_MB;
                    return;
                    
                default:
                    break;
            }
            // Should not reach here
        }
        if(intflag & I2C_INTFLAG_SB)
        {
            switch(i2c_master_context[channel].state)
            {
                case I2C_STATE_READ:
                    if(i2c_master_context[channel].size)
                    {
                        *i2c_master_context[channel].data.ptr++ = i2c->data;
                        if(--i2c_master_context[channel].size)
                        {
                            i2c->ctrlb = I2C_CTRLB_CMD_ACKREAD;
                            while(i2c->syncbusy & SYNCBUSY_SYSOP);
                        }
                        else
                        {
                            i2c->ctrlb = I2C_CTRLB_CMD_ACKSTOP
                                       | I2C_CTRLB_ACKACT // NACK
                                       ;
                            while(i2c->syncbusy & SYNCBUSY_SYSOP);
                            i2c_master_context[channel].state = I2C_STATE_DONE;
                        }
                        i2c->intflag = I2C_INTFLAG_SB;
                        return;
                    }
                    break;

                default:
                    break;
            }
            // Should not reach here
        }
    }
    
    // Should not reach here
    for(;;);
}