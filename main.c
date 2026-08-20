#include "pwm_control.h"
#include "timer_handler.h"

int main(void)
{
    PWM_SystemInit();
    Timer0_Init();

    while(1)
    {
    }
}
