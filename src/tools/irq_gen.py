import sys

BASE_ADDRESS  = 0x00000000

N_IRQ = 16 + 28
NAMES = {
    0 : "sp",
    1 : "reset",
    2 : "nmi",
    3 : "hard_fault",
    11: "svc",
    14: "pendsv",
    15: "systick"
}
HANDLERS = {
}

def irq2label(irq):
    if irq < 16:
        label = NAMES[irq]
    else:
        label = ("irq%02d"  % (irq-16))
    if irq > 0:
        label += "_handler"
    return label

def irq2address(irq):
    return BASE_ADDRESS + irq*4

print(".section .vectors")
print()

for irq in range(N_IRQ):
    if (irq >= 4 and irq <= 10) or irq == 12 or irq == 13:
        print(".word 0x00000000")
        continue
    label = irq2label(irq)
    if irq == 0:
        print(".word _estack")
    elif irq == 1:
        print(".word _start + 1")
    elif label in HANDLERS.keys():
        print("%s:\n    .word %s + 1\n"%(label, HANDLERS[label]))
    elif irq-16 in HANDLERS.keys():
        print("%s:\n    .word %s + 1\n"%(label, HANDLERS[irq-16]))
    else:
        print("%s:\n    .word 0x%08x\n"%(label, irq2address(irq) | 1))

