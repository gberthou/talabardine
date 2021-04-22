#ifndef NVM_H
#define NVM_H

#include <stdint.h>

enum nvm_calibration_e
{
    NVM_CAL_USBTRANSN,
    NVM_CAL_USBTRANSP,
    NVM_CAL_USBTRIM,
    NVM_CAL_DFLL48MCOARSE
};

uint32_t nvm_get_calibration_bits(enum nvm_calibration_e cal);

#endif

