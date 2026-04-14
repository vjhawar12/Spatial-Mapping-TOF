#include <stdint.h>
#include "tm4c1294ncpdt.h"
#include "constants.h"

// Enable interrupts
void EnableInt(void);
// Disable interrupts
void DisableInt(void);
// Low power wait
void WaitForInt(void);
// buttons 0 and 1
void PortJ_Init(void);
// buttons 2 and 3
void PortM_Init(void);
// LEDs 0 and 1
void PortN_Init(void);
// LEDs 2 and 3
void PortF_Init(void);
// Stepper motor
void PortH_Init(void);
void I2C_Init(void);
//The VL53L1X needs to be reset using XSHUT.  We will use PG0
void PortG_Init(void);
// motor timer
void Timer0_Init(void);
// led timer
void Timer1_Init(void);
static void SysTick_Init(void);