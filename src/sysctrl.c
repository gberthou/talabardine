#include <stdint.h>

#include "sysctrl.h"
#include "nvmctrl.h"
#include "nvm.h"

struct __attribute__((packed)) sysctrl_t
{
    uint32_t intenclr;
    uint32_t intenset;
    uint32_t intflag;
    uint32_t pclksr;
    uint16_t xosc;
    uint16_t _pad0;
    uint16_t xosc32k;
    uint16_t _pad1;
    uint32_t osc32k;
    uint8_t osculp32k;
    uint8_t _pad2[3];
    uint32_t osc8m;
    uint16_t dfllctrl;
    uint16_t _pad3;
    uint32_t dfllval;
    uint32_t dfllmul;
    uint8_t dfllsync;
    uint8_t _pad4[3];
    uint32_t bod33;
    uint32_t _pad5;
    uint16_t vreg;
    uint16_t _pad6;
    uint32_t vref;
    uint8_t dpllctrla;
    uint8_t _pad7[3];
    uint32_t dpllratio;
    uint32_t dpllctrlb;
    uint8_t dpllstatus;
};

#define SYSCTRL ((volatile struct sysctrl_t*) 0x40000800)

#define DFLLRDY (1 << 4)

// https://blog.thea.codes/understanding-the-sam-d21-clocks/
void sysctrl_init_DFLL48M(void)
{
    // ONDEMAND = 0 to keep the clock on
    /*
     * 17.6.7.2.2
     * DFLLCTRL.USBCRM = 1
     * DFLLCTRL.MODE = 1
     * DFLLVAL.MUX = 0xbb80 (1kHz * 48000 = 48MHz)
     * DFLLVAL.COARSE value can be loaded from NVM OTP row by software
     * DFLLCTRL.QLDIS = 0
     * DFLLCTRL.CCDIS = 1
     *
     *
     * 17.6.7.1.2
     * 1. Enable and select a reference clock (CLK_DFLL48M_REF). CLK_DFLL48M_REF is Generic Clock
     * Channel 0 (GCLK_DFLL48M_REF). Refer to GCLK - Generic Clock Controller for details.
     * 2. Select the maximum step size allowed in finding the Coarse and Fine values by writing the
     * appropriate values to the DFLL Coarse Maximum Step and DFLL Fine Maximum Step bit groups
     * (DFLLMUL.CSTEP and DFLLMUL.FSTEP) in the DFLL Multiplier register. A small step size will
     * ensure low overshoot on the output frequency, but will typically result in longer lock times. A high
     * value might give a large overshoot, but will typically provide faster locking. DFLLMUL.CSTEP and
     * DFLLMUL.FSTEP should not be higher than 50% of the maximum value of DFLLVAL.COARSE and
     * DFLLVAL.FINE, respectively.
     * 3. Select the multiplication factor in the DFLL Multiply Factor bit group (DFLLMUL.MUL) in the DFLL
     * Multiplier register. Care must be taken when choosing DFLLMUL.MUL so that the output frequency
     * does not exceed the maximum frequency of the DFLL.
     * 4. Start the closed loop mode by writing a one to the DFLL Mode Selection bit (DFLLCTRL.MODE) in
     * the DFLL Control register
     */

    /* Thanks https://blog.thea.codes/understanding-the-sam-d21-clocks/ */

    /* Set the correct number of wait states for 48 MHz @ 3.3v */
    nvmctrl_set_wait_states(1);

    /* This works around a quirk in the hardware (errata 1.2.1) -
       the DFLLCTRL register must be manually reset to this value before
       configuration. */
    while(!(SYSCTRL->pclksr & DFLLRDY));
    SYSCTRL->dfllctrl = (1 << 1); // ENABLE
    while(!(SYSCTRL->pclksr & DFLLRDY));

    /* Write the coarse and fine calibration from NVM. */
    //uint32_t coarse =
    //    ((*(uint32_t*)FUSES_DFLL48M_COARSE_CAL_ADDR) & FUSES_DFLL48M_COARSE_CAL_Msk) >> FUSES_DFLL48M_COARSE_CAL_Pos;
    uint32_t coarse = nvm_get_calibration_bits(NVM_CAL_DFLL48MCOARSE);
    /* 37.15 Notes: 1. When using DFLL48M in USB recovery mode, the Fine Step value must be Ah to guarantee a USB clock at +/-0.25% before 11ms after a resume. */
    //uint32_t fine =
    //    ((*(uint32_t*)FUSES_DFLL48M_FINE_CAL_ADDR) & FUSES_DFLL48M_FINE_CAL_Msk) >> FUSES_DFLL48M_FINE_CAL_Pos;
    uint32_t fine = 0xa;

    SYSCTRL->dfllval = (coarse << 10) | fine;

    /* Wait for the write to finish. */
    while(!(SYSCTRL->pclksr & DFLLRDY));


    SYSCTRL->dfllctrl |= (1 << 5) // USBCRM
                       | (1 << 8) // CCDIS
                       ;
    /* Disable chill cycle as per datasheet to speed up locking.
       This is specified in section 17.6.7.2.2, and chill cycles
       are described in section 17.6.7.2.1. */

    /* Configure the DFLL to multiply the 1 kHz clock to 48 MHz */
    SYSCTRL->dfllmul = 48000     // (1kHz * 48000 = 48MHz)
                     | (1 << 16) // FSTEP
                     | (1 << 26) // CSTEP
                     ;

    /* Closed loop mode */
    SYSCTRL->dfllctrl |= (1 << 2); // MODE

    /* Enable the DFLL */
    SYSCTRL->dfllctrl= (1 << 1); // ENABLE

    /* Wait for the write to complete */
    while(!(SYSCTRL->pclksr & DFLLRDY));
}

