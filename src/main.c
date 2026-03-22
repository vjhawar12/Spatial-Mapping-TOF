#include <stdint.h>
#include "tm4c1294ncpdt.h"
#include "SysTick.h"
#include "PLL.h"
#include "tof.h"
#include "uart.h"

#define STEPS_PER_REV 2048
#define STEP_11_25 64
#define STEP_45  256

// 10ms delay
#define DELAY 1
#define LED_BLINK_DELAY 2
#define BUTTON_DEBOUNCING 2


enum State {
	FORWARD,
	STOP,
	REVERSE,
	HOME
}; 

volatile enum State state = STOP;

// forward direction: increment pattern index
int full_step_pattern[] = { 
	0x3, 0x6, 0xC, 0x9
}; 


int pattern_index = 0;
int pos = 0;

// volatile because they're modified in ISRs and read outside of them
volatile int blink_div = STEP_11_25;
volatile int dir = 0; // 1: forward, 0: reverse
volatile int led2_on = 1; 


int distance = 0;

// Enable interrupts
void EnableInt(void) {    
	__asm("    cpsie   i\n");
}

// Disable interrupts
void DisableInt(void) {    
	__asm("    cpsid   i\n");
}

// Low power wait
void WaitForInt(void) {    
	__asm("    wfi\n");
}

// buttons 0 and 1
void PortJ_Init(void){	
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R8; // Activate the clock for Port J
	while((SYSCTL_PRGPIO_R & SYSCTL_PRGPIO_R8) == 0){};	// Allow time for clock to stabilize
  
	GPIO_PORTJ_DIR_R &= ~0x3; // Enable PJ[0-1] as inputs 
	GPIO_PORTJ_DEN_R |= 0x3; // Enable PJ[0-1] as digital pins
	GPIO_PORTJ_AFSEL_R &= ~0x3; // disabling alternate function
	GPIO_PORTJ_AMSEL_R &= ~0x3; // disabling analog functionality
	GPIO_PORTJ_PUR_R |= 0x3; // enabling internal pull up 
	GPIO_PORTJ_IS_R &= ~0x3; // enabling edge sensitive not level sensitive
	GPIO_PORTJ_IBE_R  &= ~0x3; // dont want both edges to throw interrputs
	GPIO_PORTJ_IEV_R &= ~0x3; // only letting falling edges throw interrupts 
	GPIO_PORTJ_ICR_R = 0x3; // clearing any pending interrupts
	GPIO_PORTJ_IM_R |= 0x3; //  actually enabling interrupts from this port
	
	// IRQ for portJ: 51
	// ENn# = IRQ / 32 = 1
	// Bit# = IRQ % 32 = 19
	NVIC_EN1_R |= (1 << 19); 
	return;
}

// buttons 2 and 3
void PortM_Init(void){	
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R11;		              // Activate the clock for Port M
	while((SYSCTL_PRGPIO_R & SYSCTL_PRGPIO_R11) == 0){};	      // Allow time for clock to stabilize
  
	GPIO_PORTM_DIR_R &= ~0x3;														// Enable PM[0-1] as inputs 
	GPIO_PORTM_DEN_R |= 0x3;                        		// Enable PM[0-1] as digital pins
	GPIO_PORTM_AFSEL_R &= ~0x3;
	GPIO_PORTM_AMSEL_R &= ~0x3;
	GPIO_PORTM_PUR_R |= 0x3;
	GPIO_PORTM_IS_R &= ~0x3;
	GPIO_PORTM_IBE_R  &= ~0x3; // dont want both edges to throw interrputs
	GPIO_PORTM_IEV_R &= ~0x3; // only letting falling edges throw interrupts
	GPIO_PORTM_ICR_R = 0x3; // clearing any pending interrupts  
	GPIO_PORTM_IM_R |= 0x3; //  actually enabling interrupts from this port

	// IRQ for PortM: 72
	// ENn# = IRQ / 32 = 2
	// Bit# = IRQ % 32 = 8
	NVIC_EN2_R |= (1 << 8); 
	return;
}

// LEDs 0 and 1
void PortN_Init(void){	
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R12;		              // Activate the clock for Port N
	while((SYSCTL_PRGPIO_R & SYSCTL_PRGPIO_R12) == 0){};	      // Allow time for clock to stabilize
  
	GPIO_PORTN_DIR_R |= 0x3;														// Enable PN[0-1] as outputs 
	GPIO_PORTN_DEN_R |= 0x3;                        		// Enable PN[0-1] as digital pins
	return;
}


// LEDs 2 and 3
void PortF_Init(void){	
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R5;		              // Activate the clock for Port N
	while((SYSCTL_PRGPIO_R & SYSCTL_PRGPIO_R5) == 0){};	      // Allow time for clock to stabilize
	
	// unlocking portF
	GPIO_PORTF_LOCK_R = 0x4C4F434B;
	GPIO_PORTF_CR_R |= 0x01;   // allow changes to PF0
  
	GPIO_PORTF_DIR_R |= 0x11;														// Enable PF0 & PF4 as outputs 
	GPIO_PORTF_DEN_R |= 0x11;                        		// Enable PF0 & PF4 as digital pins
	return;
}

// Stepper motor
void PortH_Init(void){
	//Use PortH pins (PH0-PH3) for output
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R7;		// activate clock for Port H
	while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R7) == 0){};	// allow time for clock to stabilize
	GPIO_PORTH_DIR_R |= 0x0F;        			// configure Port H pins (PH0-PH3) as output
	GPIO_PORTH_AFSEL_R &= ~0x0F;     				// disable alt funct on Port H pins (PH0-PH3)
	GPIO_PORTH_DEN_R |= 0x0F;        				// enable digital I/O on Port H pins (PH0-PH3)
	GPIO_PORTH_AMSEL_R &= ~0x0F;     				// disable analog functionality on Port H pins (PH0-PH3)
	return;
}

void I2C_Init(void){
  SYSCTL_RCGCI2C_R |= SYSCTL_RCGCI2C_R0;           													// activate I2C0
  SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R1;          												// activate port B
  while((SYSCTL_PRGPIO_R&0x0002) == 0){};																		// ready?

    GPIO_PORTB_AFSEL_R |= 0x0C;           																	// 3) enable alt funct on PB2,3       0b00001100
    GPIO_PORTB_ODR_R |= 0x08;             																	// 4) enable open drain on PB3 only

    GPIO_PORTB_DEN_R |= 0x0C;             																	// 5) enable digital I/O on PB2,3
//    GPIO_PORTB_AMSEL_R &= ~0x0C;          																// 7) disable analog functionality on PB2,3

                                                                            // 6) configure PB2,3 as I2C
//  GPIO_PORTB_PCTL_R = (GPIO_PORTB_PCTL_R&0xFFFF00FF)+0x00003300;
  GPIO_PORTB_PCTL_R = (GPIO_PORTB_PCTL_R&0xFFFF00FF)+0x00002200;    //TED
    I2C0_MCR_R = I2C_MCR_MFE;                      													// 9) master function enable
    I2C0_MTPR_R = 0b0000000000000101000000000111011;                       	// 8) configure for 100 kbps clock (added 8 clocks of glitch suppression ~50ns)
//    I2C0_MTPR_R = 0x3B;                                        						// 8) configure for 100 kbps clock
        
}

//The VL53L1X needs to be reset using XSHUT.  We will use PG0
void PortG_Init(void){
    //Use PortG0
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R6;                // activate clock for Port N
    while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R6) == 0){};    // allow time for clock to stabilize
    GPIO_PORTG_DIR_R &= 0x00;                                        // make PG0 in (HiZ)
  GPIO_PORTG_AFSEL_R &= ~0x01;                                     // disable alt funct on PG0
  GPIO_PORTG_DEN_R |= 0x01;                                        // enable digital I/O on PG0
                                                                                                    // configure PG0 as GPIO
  //GPIO_PORTN_PCTL_R = (GPIO_PORTN_PCTL_R&0xFFFFFF00)+0x00000000;
  GPIO_PORTG_AMSEL_R &= ~0x01;                                     // disable analog functionality on PN0

    return;
}


void turn_on_led1() {
	GPIO_PORTN_DATA_R |= 0x1;
}

void turn_off_led1() {
	GPIO_PORTN_DATA_R &= ~0x1;
}

void turn_on_led0() {
	GPIO_PORTN_DATA_R |= 0x2;
}

void turn_off_led0() {
	GPIO_PORTN_DATA_R &= ~0x2;
}

void turn_on_led3() {
	GPIO_PORTF_DATA_R |= 0x1;
}

void turn_off_led3() {
	GPIO_PORTF_DATA_R &= ~0x1;
}

void turn_on_led2() {
	GPIO_PORTF_DATA_R |= 0x10;
}

void turn_off_led2() {
	GPIO_PORTF_DATA_R &= ~0x10;
}


void stop() {
	GPIO_PORTH_DATA_R = 0b0;		
}

void full_step_once_forward(){
	GPIO_PORTH_DATA_R = full_step_pattern[pattern_index];
	pattern_index++;
	pattern_index &= 3;
	pos++;
	pos = pos % STEPS_PER_REV;
}

void full_step_once_reverse(){
	pattern_index += 3;
	pattern_index &= 3;
	GPIO_PORTH_DATA_R = full_step_pattern[pattern_index];
	pos--;
	pos = (pos + STEPS_PER_REV) % STEPS_PER_REV; 
}

void scan(int i) {
	distance = 0;
	for (int j = 0; j < 10; j++) {
		distance += tof_get_distance();
	}
	distance = distance / 10;
	sprintf(printf_buffer, "Angle %d Distance %d \r\n", (i / STEP_45) * 45, distance);
	UART_printf(printf_buffer);
}

enum State forward_full_step() {
	scan(0);
	for (int i = 1; i <= STEPS_PER_REV; i++) {
		if (state != FORWARD) {
			// early return if button push changes state
			return state;
		} 
		led2_on? turn_on_led2() : turn_off_led2();
		full_step_once_forward();
		if (i % STEP_45 == 0 && i != STEPS_PER_REV) {
			scan(i);
		}
		if (i % blink_div == 0) {
			turn_on_led3();
			SysTick_Wait10ms(LED_BLINK_DELAY);
			turn_off_led3();
		}
		SysTick_Wait10ms(DELAY);
	}
	return STOP;
}


enum State reverse_full_step() {
	for (int i = 1; i <= STEPS_PER_REV; i++) {
		if (state != REVERSE) {
			// early return if button push changes state
			return state;
		} 
		led2_on? turn_on_led2() : turn_off_led2();
		full_step_once_reverse();
		if (i % blink_div == 0) {
			turn_on_led3();
			SysTick_Wait10ms(LED_BLINK_DELAY);
			turn_off_led3();
		}
		SysTick_Wait10ms(DELAY);
	}
	return STOP;
}

void reverse_until_home() {
	while (pos != 0) {
		full_step_once_reverse();
		SysTick_Wait10ms(DELAY);
	}
}

void forward_until_home() {
	while (pos != 0) {
		full_step_once_forward();
		SysTick_Wait10ms(DELAY);
	}
}

enum State home() {
	int diff1 = pos - 0;
	int diff2 = STEPS_PER_REV - pos;
	
	diff1 < diff2? reverse_until_home() : forward_until_home();
	return STOP;
}

// called in while(true) loop 
void StateMachine() {
	switch(state) {
		case STOP:
			stop();
			turn_off_led0();
			turn_off_led2();
			turn_off_led3();
			break;
		case FORWARD:
			turn_on_led0();
			turn_on_led1();
			state = forward_full_step();
			break;
		case REVERSE:
			turn_on_led0();
			turn_off_led1();
			state = reverse_full_step();
			break;
		case HOME:
			turn_off_led0();
			turn_off_led1();
			turn_off_led2();
			turn_off_led3();
			state = home();
			stop();
			break;
	}
}

// interrupt-based approach will make button0 call this upon press
void HandleButton0Press() {
	if (state == STOP) {
		state = dir? FORWARD : REVERSE;
	} else {
		state = STOP;
	}
}

// interrupt-based approach will make button1 call this upon press
void HandleButton1Press() {
	dir = !dir;
	if (state == FORWARD && dir == 0) {
		state = REVERSE;
		turn_off_led1();
	} else if (state == REVERSE && dir == 1) {
		state = FORWARD;
		turn_on_led1();
	}
}

// interrupt-based approach will make button2 call this upon press
void HandleButton2Press() {
	if (blink_div == STEP_11_25) {
		blink_div = STEP_45;
		led2_on = 0;
	} else {
		blink_div = STEP_11_25;
		led2_on = 1;
	}
}

// interrupt-based approach will make button3 call this upon press
void HandleButton3Press() {
	state = HOME;
}

void GPIOJ_IRQHandler(void) {
	uint32_t mis = GPIO_PORTJ_MIS_R;
	if (mis & 0x1) { // button0 pressed
		GPIO_PORTJ_ICR_R = 0x1;
		SysTick_Wait10ms(2); // button debouncing
		HandleButton0Press();

	} if (mis & 0x2) { // button1 pressed
		GPIO_PORTJ_ICR_R = 0x2;
		SysTick_Wait10ms(2); // button debouncing
		HandleButton1Press();
	}
}

void GPIOM_IRQHandler(void) {
	uint32_t mis = GPIO_PORTM_MIS_R;
	GPIO_PORTM_ICR_R = 0x3;
	SysTick_Wait10ms(BUTTON_DEBOUNCING); 
	
	if ((mis & 0x1) && ((GPIO_PORTM_DATA_R & 0x1) == 0)) { // button2 pressed	
		HandleButton2Press();
	} else if ((mis & 0x2) && ((GPIO_PORTM_DATA_R & 0x2) == 0)) { // button3 pressed
		HandleButton3Press();
	}
}


int main(void) {
	PLL_Init();
	SysTick_Init();
	PortF_Init();
	PortH_Init();
	PortN_Init();
	PortM_Init();
	PortJ_Init();
	PortG_Init();
	EnableInt();
	UART_Init();
	I2C_Init();

	tof_init();

	while (1) {
		StateMachine();
	}

}

/*
Suggested Improvements / Code Review Notes

Architecture
------------
- Avoid blocking motor control loops (forward_full_step / reverse_full_step).
  Instead, step the motor once per iteration of the main loop. This makes the
  system more responsive to interrupts and simplifies stop, reverse, and home logic.
#ifndef TOF_H
#define TOF_H
- Centralize hardware control. Currently LEDs and motor outputs are modified
  from multiple places (state machine, motion loops, and handlers). Prefer a
  single control layer responsible for updating outputs.

- Avoid redundant state variables. `led2_on` duplicates information already
  implied by `blink_div`. LED2 state could be derived directly from the current
  angle mode.

Interrupt Handling
------------------
- Do not use blocking delays inside ISRs (SysTick_Wait10ms in GPIO handlers).
  ISRs should execute quickly. Instead:
      1. Clear the interrupt
      2. Set an event flag
      3. Handle debouncing and logic in the main loop

- ISRs should ideally only signal events (button presses) and not directly
  modify system state.

State Machine Design
--------------------
- The state machine is generally clear but state ownership is scattered.
  Prefer a design where:
      - ISRs only generate events
      - The main loop processes events and performs state transitions
      - Hardware outputs are updated in one place.

- `home()` always returns STOP, so its return value is unnecessary.
  Consider changing it to `void home(void)` or consistently using the return value.

Stepper Motor Control
---------------------
- Motor rotation is implemented as long blocking loops (up to 2048 steps).
  This reduces responsiveness to button presses. A better approach is to
  execute one step per loop iteration and let the state machine control motion.

- Shortest-path home logic works but would benefit from clearer naming
  (e.g., `reverse_distance` and `forward_distance`) and comments.

Code Readability
----------------
- Some variables have unclear names (e.g., `dir`, `diff1`, `diff2`).
  More descriptive names improve maintainability.

- Several GPIO bit masks are "magic numbers" (0x3, 0x11, etc.).
  These should ideally be replaced with named constants.

General Embedded Practices
--------------------------
- Separate board-level hardware configuration from application logic where possible.
- Keep ISRs minimal and deterministic.
- Minimize duplicated state and derive behavior from existing variables.
*/
