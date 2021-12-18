#ifndef NVIC_H
#define NVIC_H

struct __attribute__((packed)) nvic_t
{
    uint32_t iser;
    uint32_t _pad0[31];
    uint32_t icer;
    uint32_t _pad1[31];
    uint32_t ispr;
    uint32_t _pad2[31];
    uint32_t icpr;
    uint32_t _pad3[95];
    uint32_t ipr[8];
};

enum nvic_interrupt_e
{
    NVIC_EIC = 4,
    NVIC_USB = 7,
    NVIC_SERCOM0 = 9,
    NVIC_TC3 = 18
};

#define NVIC ((volatile struct nvic_t*) 0xe000e100)

#define nvic_enable(id)  (NVIC->iser = (1u << (id)))
#define nvic_disable(id) (NVIC->icer = (1u << (id)))
#define nvic_clear(id)   (NVIC->icpr = (1u << (id)))
#define nvic_set_priority(id, pr) do \
{ \
    volatile uint32_t *pipr = &NVIC->ipr[(id) >> 2]; \
    uint32_t ipr = *pipr; \
    uint32_t shift = (((id) & 0x3) << 3); \
    ipr &= ~(0xff << shift); \
    ipr |= (((pr) & 0xff) << shift); \
    *pipr = ipr; \
} while (0);



#endif

