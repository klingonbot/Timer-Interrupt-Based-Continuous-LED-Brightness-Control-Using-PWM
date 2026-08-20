#include <stdint.h>
#include <stdbool.h>

#include "driverlib/sysctl.h"

#include "pwm_control.h"
#include "timer_handler.h"

int main(void)
{
    SysCtlClockSet(SYSCTL_SYSDIV_5 |
                   SYSCTL_USE_PLL |
                   SYSCTL_OSC_MAIN |
                   SYSCTL_XTAL_16MHZ);

    PWM_Init();

    Timer0_Init();

    while(1)
    {
    }
}
