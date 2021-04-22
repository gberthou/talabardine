#include "nvmctrl.h"

struct __attribute__((packed)) nvmctrl_t
{
    uint16_t ctrla;
    uint16_t _pad0;
    uint32_t ctrlb;
    uint32_t param;
    uint8_t intenclr;
    uint8_t _pad1[3];
    uint8_t intenset;
    uint8_t _pad2[3];
    uint8_t intflag;
    uint8_t _pad3[3];
    uint16_t status;
    uint16_t _pad4;
    uint32_t addr;
    uint16_t lock;
};

#define NVMCTRL ((volatile struct nvmctrl_t*) 0x41004000)

void nvmctrl_set_wait_states(uint8_t waitstates)
{
    waitstates &= 0xf;

    uint32_t ctrlb = NVMCTRL->ctrlb;
    ctrlb &= ~0x1e;
    ctrlb |= (waitstates << 1); // RWS
    NVMCTRL->ctrlb = ctrlb;
}

