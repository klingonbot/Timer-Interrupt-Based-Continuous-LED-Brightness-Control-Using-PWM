#include <stdint.h>

#include "inc/hw_memmap.h"

#include "driverlib/gpio.h"
#include "driverlib/timer.h"
#include "driverlib/interrupt.h"

#include "pwm_control.h"
#include "timer_handler.h"

void Timer0_Init(void)
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);

    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_TIMER0));

    TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);

    TimerLoadSet(TIMER0_BASE,
                 TIMER_A,
                 400000 - 1);

    TimerIntEnable(TIMER0_BASE,
                   TIMER_TIMA_TIMEOUT);

    IntEnable(INT_TIMER0A);

    IntMasterEnable();

    TimerEnable(TIMER0_BASE,
                TIMER_A);
}

void Timer0A_Handler(void)
{
    TimerIntClear(TIMER0_BASE,
                  TIMER_TIMA_TIMEOUT);

    uint32_t buttonState =
        GPIOPinRead(GPIO_PORTF_BASE,
                    GPIO_PIN_0 | GPIO_PIN_4);

    if((buttonState & GPIO_PIN_4) == 0)
    {
        if(ui32DutyCycle < (PWM_PERIOD - PWM_STEP))
            ui32DutyCycle += PWM_STEP;
        else
            ui32DutyCycle = PWM_PERIOD - 1;
    }

    if((buttonState & GPIO_PIN_0) == 0)
    {
        if(ui32DutyCycle > PWM_STEP)
            ui32DutyCycle -= PWM_STEP;
        else
            ui32DutyCycle = 1;
    }

    PWMPulseWidthSet(PWM1_BASE,
                     PWM_OUT_5,
                     ui32DutyCycle);
}
