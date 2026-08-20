#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_ints.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/pwm.h"
#include "driverlib/pin_map.h"
#include "driverlib/interrupt.h"
#include "driverlib/timer.h"
#define PWM_PERIOD 40000
#define PWM_STEP 400
volatile uint32_t ui32DutyCycle = 1;
void Timer0A_Handler(void);
int main(void) {
SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL |
SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);
SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM1);
SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
HWREG(GPIO_PORTF_BASE + GPIO_O_CR) |= GPIO_PIN_0;
GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_4);
GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_4,
GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
SysCtlPWMClockSet(SYSCTL_PWMDIV_1);
GPIOPinConfigure(GPIO_PF1_M1PWM5);
GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_1);


PWMGenConfigure(PWM1_BASE, PWM_GEN_2, PWM_GEN_MODE_DOWN);
PWMGenPeriodSet(PWM1_BASE, PWM_GEN_2, PWM_PERIOD);
PWMPulseWidthSet(PWM1_BASE, PWM_OUT_5, ui32DutyCycle);
PWMGenEnable(PWM1_BASE, PWM_GEN_2);
PWMOutputState(PWM1_BASE, PWM_OUT_5_BIT, true);
TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
TimerLoadSet(TIMER0_BASE, TIMER_A, 400000 - 1);
TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
IntEnable(INT_TIMER0A);
IntMasterEnable();
TimerEnable(TIMER0_BASE, TIMER_A);
while(1) { }
}
void Timer0A_Handler(void) {
TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
uint32_t buttonState = GPIOPinRead(GPIO_PORTF_BASE,
GPIO_PIN_0 | GPIO_PIN_4);
if ((buttonState & GPIO_PIN_4) == 0) {
if (ui32DutyCycle < (PWM_PERIOD - PWM_STEP))
ui32DutyCycle += PWM_STEP;
else
ui32DutyCycle = PWM_PERIOD - 1;
}
if ((buttonState & GPIO_PIN_0) == 0) {
if (ui32DutyCycle > PWM_STEP)
ui32DutyCycle -= PWM_STEP;
else
ui32DutyCycle = 1;
}
PWMPulseWidthSet(PWM1_BASE, PWM_OUT_5, ui32DutyCycle);
}
