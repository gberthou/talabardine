#ifndef NVIC_H
#define NVIC_H

struct nvic_t
{
    uint32_t iser;
    uint32_t _pad0[31];
    uint32_t icer;
    uint32_t _pad1[31];
    uint32_t ispr;
    uint32_t _pad2[31];
    uint32_t icpr;
};

enum nvic_interrupt_e
{
    NVIC_EIC = 4,
    NVIC_TC3 = 18
};

#define NVIC ((volatile struct nvic_t*) 0xe000e100)

#define nvic_enable(id)  (NVIC->iser = (1u << (id)))
#define nvic_disable(id) (NVIC->icer = (1u << (id)))
#define nvic_clear(id)   (NVIC->icpr = (1u << (id)))

#endif

