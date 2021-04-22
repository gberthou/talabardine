#include <stddef.h>

#include "talabardine.h"
#include "sercom.h"

int main(void)
{
    talabardine_init();
    for(;;)
    {
        //uint16_t pressure =
        talabardine_get_pressure();
        //uart_display_half(pressure);
    }
    return 0;
}

