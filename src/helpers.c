#include <stdint.h>
#include <stddef.h>

unsigned int __aeabi_uidivmod(unsigned int numerator, unsigned int denominator)
{
    unsigned int quot = 0;
    for(; numerator >= denominator; ++quot, numerator -= denominator);
    __asm__ __volatile__("mov r1, %0" :: "r"(numerator));
    return quot;
}

unsigned int __aeabi_uidiv(unsigned int numerator, unsigned int denominator)
{
    unsigned int quot = 0;
    if(denominator == 1)
        return numerator;
    for(; numerator >= denominator; ++quot, numerator -= denominator);
    return quot;
}

void *memset(void *ptr, int value, size_t num)
{
    uint8_t *dst = ptr;
    while(num--)
        *dst++ = value;
    return ptr;
}

void *memcpy(void *destination, const void *source, size_t num )
{
    uint8_t *dst = destination;
    const uint8_t *src = source;
    while(num--)
        *dst++ = *src++;
    return destination;
}

size_t strlen(const char *str)
{
    size_t n = 0;
    while(*str++)
        ++n;
    return n;
}

