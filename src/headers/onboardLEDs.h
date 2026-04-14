#include <stdint.h>
#include "tm4c1294ncpdt.h"
#include "PLL.h"
#include "SysTick.h"
#include "constants.h"

#define DELAY 1

void FlashLED1(int count);
void FlashLED2(int count);
void FlashLED3(int count);
void FlashLED4(int count);
void FlashAllLEDs(void);
void FlashI2CError(int count);
void FlashI2CTx(void);
void FlashI2CRx(void);
void onboardLEDs_Init(void);
void turn_on_led1(void) {}
void turn_off_led1(void) {}
void turn_on_led0(void) {}
void turn_off_led0(void) {}
void flash_led3(void) {}
void turn_on_led2(void) {}
void turn_off_led2(void) {}