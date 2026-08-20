# ⚡ Timer-Interrupt PWM LED Brightness Control — TM4C123GH6PM

[![Platform](https://img.shields.io/badge/platform-TM4C123GH6PM-blue)]()
[![Toolchain](https://img.shields.io/badge/IDE-Code%20Composer%20Studio-orange)]()
[![Library](https://img.shields.io/badge/library-TivaWare-lightgrey)]()


An interrupt-driven PWM brightness controller for the **TI Tiva C TM4C123GH6PM
LaunchPad**. A hardware PWM generator drives the on-board red LED at 1 kHz,
while a 100 Hz Timer0A interrupt polls the two on-board push buttons and
adjusts the duty cycle in real time — the CPU main loop stays completely idle
the whole time (`while(1) {}`, no polling, no delays).

<p align="center">
  <img src="https://img.shields.io/badge/PWM-1kHz-informational" />
  <img src="https://img.shields.io/badge/Sampling-100Hz-informational" />
  <img src="https://img.shields.io/badge/Resolution-1%25%20step-informational" />
</p>

---

## Table of Contents

- [Features](#features)
- [Hardware](#hardware)
- [How It Works](#how-it-works)
- [Repo Structure](#repo-structure)
- [Getting Started](#getting-started)
- [Usage](#usage)
- [Code](#code)
- [Results](#results)
- [Conclusions](#conclusions)
- [Possible Improvements](#possible-improvements)
- [References](#references)
- [Authors](#authors)
- [License](#license)

---

## Features

- 🎛️ Smooth, step-wise LED brightness control (1% per step, 100 discrete levels)
- ⏱️ Fully interrupt-driven — zero CPU time spent in the main loop
- 🚫 No software debounce needed — 10 ms sampling window naturally suppresses switch bounce
- 🔓 Handles the PF0 NMI lock/unlock required to use SW2 as a GPIO input
- 🧱 Built entirely on TivaWare driverlib — no bare-metal register hacking required

## Hardware

| Component | Pin | Notes |
|---|---|---|
| On-board red LED | `PF1` | Alternate function `M1PWM5` |
| Push button SW1 | `PF4` | Active-low, internal pull-up, brightness **up** |
| Push button SW2 | `PF0` | Active-low, internal pull-up, NMI-locked at reset, brightness **down** |

No external circuitry is required — this runs entirely on the on-board
LaunchPad LED and buttons.

## How It Works

```
 40 MHz sysclk
      │
      ├── PWM1 Gen2 (count-down, 40,000-tick period) ──► PF1 (M1PWM5) @ 1 kHz
      │
      └── Timer0A (periodic, 399,999-tick load) ──► ISR @ 100 Hz
                                                        │
                                        reads PF0 + PF4 ┤
                                                        │
                                  adjusts ui32DutyCycle ┤
                                                        │
                                  PWMPulseWidthSet() ───┘
```

- **PWM:** PWM1 Generator 2, count-down mode, 40,000-tick period @ 40 MHz → 1 kHz on PF1.
- **Sampling:** Timer0A periodic mode, load value `399,999` → 10 ms / 100 Hz interrupt.
- **On each interrupt:**
  - `PF4 == 0` (SW1 held) → duty cycle `+= 400` ticks, capped at `PWM_PERIOD - 1`
  - `PF0 == 0` (SW2 held) → duty cycle `-= 400` ticks, floored at `1`
  - New value written straight to the PWM compare register via `PWMPulseWidthSet`
- **Debounce:** the 10 ms interrupt period exceeds typical mechanical bounce time (1–5 ms), so no explicit debounce logic is needed.
- **Thread safety:** `ui32DutyCycle` is declared `volatile` to guarantee correct read/write behavior across the ISR and main-loop execution contexts.

## Repo Structure

```
.
├── README.md
├── src/
│   └── main.c              # PWM + Timer0A ISR implementation
├── inc/                     # TivaWare hardware headers (project-local, if vendored)
└── docs/
    └── esd_project_report.pdf   # Full project write-up
```

> Adjust this section to match your actual folder layout before committing.

## Getting Started

### Prerequisites

- [Code Composer Studio (CCS)](https://www.ti.com/tool/CCSTUDIO)
- [TivaWare Peripheral Driver Library](https://www.ti.com/tool/SW-TM4C)
- TM4C123GH6PM LaunchPad (EK-TM4C123GXL) + USB cable

### Build

1. Clone this repo:
   ```bash
   git clone https://github.com/<your-username>/<your-repo>.git
   ```
2. Open the project in CCS and point it at your local TivaWare install.
3. Build (`Project → Build`).
4. Flash to the LaunchPad via the on-board ICDI debugger (`Run → Debug` or `Run → Load`).

## Usage

- Hold **SW1** → LED brightness increases (1% every 10 ms).
- Hold **SW2** → LED brightness decreases (1% every 10 ms).
- Release both → brightness holds at its last value.

## Code

<details>
<summary>main.c (click to expand)</summary>

```c
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
#define PWM_STEP   400

volatile uint32_t ui32DutyCycle = 1;
void Timer0A_Handler(void);

int main(void) {
    SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL |
                   SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM1);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);

    // Unlock PF0 (NMI pin) so it can be used as a GPIO input
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

    while (1) { }
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
```

</details>

## Results

- 100 discrete brightness levels (1% resolution) across the full PWM range (`PWM_PERIOD = 40000`, `PWM_STEP = 400`).
- The 10 ms Timer0A period doubles as an implicit debounce window — no extra debounce circuitry or delay code required.
- Main loop stays fully unblocked, confirming clean separation between peripheral handling (ISR) and application logic.

## Conclusions

Timer-based polling inside a periodic ISR decouples input sampling from the main execution
context. The fixed 10 ms sampling interval provides inherent debounce suppression, eliminating
the need for GPIO edge-triggered interrupts and associated debounce countermeasures.
Delegating the PWM waveform to the dedicated PWM hardware module removes any CPU
involvement in signal generation. The `volatile` qualifier on `ui32DutyCycle` ensures correct
read/write behavior across the ISR and main-loop execution contexts. The architecture scales to
multi-channel PWM control or higher sampling rates by adjusting the timer load value and step
size independently.

## Possible Improvements

- [ ] Add exponential/gamma-corrected brightness curve for perceptually linear dimming
- [ ] Support long-press auto-repeat acceleration
- [ ] Port duty-cycle state to non-volatile storage to persist brightness across resets
- [ ] Extend to RGB LED with independent channel control

## References

1. Texas Instruments, *TM4C123GH6PM Microcontroller Datasheet*, Document SPMS376E.
2. Texas Instruments, *TivaWare Peripheral Driver Library User Guide*, Document SW-TM4C-DRL-UG.

## Authors

- **Roopeshwar Megadula** — 23UEC604 — Electronics and Communication Engineering, LNMIIT Jaipur
- **Sai Dathathreya Karimisetty** — 23DEC507 — Electronics and Communication Engineering, LNMIIT Jaipur



---

