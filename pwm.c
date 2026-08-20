#include <stdint.h>
#include <stdbool.h>

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"

#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/pwm.h"
#include "driverlib/pin_map.h"

#include "pwm_control.h"

volatile uint32_t ui32DutyCycle = 1;

void PWM_Init(void)
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM1);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_PWM1));
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF));

    HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
    HWREG(GPIO_PORTF_BASE + GPIO_O_CR) |= GPIO_PIN_0;

    GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_4);

    GPIOPadConfigSet(GPIO_PORTF_BASE,
                     GPIO_PIN_0 | GPIO_PIN_4,
                     GPIO_STRENGTH_2MA,
                     GPIO_PIN_TYPE_STD_WPU);

    SysCtlPWMClockSet(SYSCTL_PWMDIV_1);

    GPIOPinConfigure(GPIO_PF1_M1PWM5);
    GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_1);

    PWMGenConfigure(PWM1_BASE,
                    PWM_GEN_2,
                    PWM_GEN_MODE_DOWN);

    PWMGenPeriodSet(PWM1_BASE,
                    PWM_GEN_2,
                    PWM_PERIOD);

    PWMPulseWidthSet(PWM1_BASE,
                     PWM_OUT_5,
                     ui32DutyCycle);

    PWMGenEnable(PWM1_BASE, PWM_GEN_2);

    PWMOutputState(PWM1_BASE,
                   PWM_OUT_5_BIT,
                   true);
}
