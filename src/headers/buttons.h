#include <stdint.h>
#include "stepper.h"

volatile int button0_pressed = 0;
volatile int button1_pressed = 0;
volatile int button2_pressed = 0;
volatile int button3_pressed = 0;
volatile uint32_t last_time_buttton0_pressed = 0;
volatile uint32_t last_time_buttton1_pressed = 0;
volatile uint32_t last_time_buttton2_pressed = 0;
volatile uint32_t last_time_buttton3_pressed = 0;

// interrupt-based approach will make button0 call this upon press
void HandleButton0Press();
// interrupt-based approach will make button1 call this upon press
void HandleButton1Press();
// interrupt-based approach will make button2 call this upon press
void HandleButton2Press();
// interrupt-based approach will make button3 call this upon press
void HandleButton3Press();