#include <stddef.h>

#include "nvm.h"

#define NVM_CALIBRATION ((volatile const uint32_t*) 0x00806020)

// Warning: doesn't work with bitfields that cross 32bit values:
//  - ADC LINEARITY (34:27)
static inline uint32_t nvm_get_bits(size_t from, size_t to)
{
    uint32_t tmp = NVM_CALIBRATION[from >> 5];
    tmp >>= (from & 0x1f);

    uint32_t mask = 0;
    for(size_t i = 0; i < to - from + 1; ++i)
        mask |= (1 << i);
    return (tmp & mask);
}

uint32_t nvm_get_calibration_bits(enum nvm_calibration_e cal)
{
    switch(cal)
    {
        case NVM_CAL_DFLL48MCOARSE:
            return nvm_get_bits(58, 63);

        default:
            return 0xdeadbeef;
    }
}

