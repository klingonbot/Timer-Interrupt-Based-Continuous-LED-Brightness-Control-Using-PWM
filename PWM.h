#ifndef PWM_CONTROL_H
#define PWM_CONTROL_H

#include <stdint.h>

#define PWM_PERIOD 40000
#define PWM_STEP   400

extern volatile uint32_t ui32DutyCycle;

void PWM_SystemInit(void);

#endif
