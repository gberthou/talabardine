#include "utils.h"
#include "sercom.h"
#include "config.h"

void dump(const void *_src, size_t size)
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
