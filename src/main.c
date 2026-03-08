#include <stdint.h>
#include "tm4c1294ncpdt.h"
#include "SysTick.h"
#include "PLL.h"


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
volatile int homed = 0; // 0: not at home, 1: at home

// volatile because they're modified in ISRs and read outside of them
volatile int blink_div = STEP_11_25;
volatile int dir = 1; // 1: forward, 0: reverse

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


enum State forward_full_step() {
	for (int i = 1; i <= STEPS_PER_REV; i++) {
		if (state != FORWARD) {
			// early return if button push changes state
			return state;
		} 
		full_step_once_forward();
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

enum State home() {
	while (pos) {
		if (state != HOME) {
			return state;
		}
		full_step_once_reverse();
		SysTick_Wait10ms(DELAY);
	}
	homed = 1;
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
			dir && !homed? turn_on_led1() : turn_off_led1();
			break;
		case FORWARD:
			homed = 0;
			turn_on_led0();
			turn_on_led1();
			blink_div == STEP_11_25? turn_on_led2() : turn_off_led2();
			state = forward_full_step();
			break;
		case REVERSE:
			homed = 0;
			turn_on_led0();
			turn_off_led1();
			blink_div == STEP_11_25? turn_on_led2() : turn_off_led2();
			state = reverse_full_step();
			break;
		case HOME:
			turn_off_led0();
			turn_off_led1();
			turn_off_led2();
			turn_off_led3();
			stop();
			state = home();
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
	} else if (state == REVERSE && dir == 1) {
		state = FORWARD;
	}
}

// interrupt-based approach will make button3 call this upon press
void HandleButton2Press() {
	if (blink_div == STEP_11_25) {
		blink_div = STEP_45;
	} else {
		blink_div = STEP_11_25;
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
	if (mis & 0x1) { // button3 pressed
		GPIO_PORTM_ICR_R = 0x1;
		SysTick_Wait10ms(BUTTON_DEBOUNCING); 
		HandleButton3Press();

	} if (mis & 0x2) { // button2 pressed	
		GPIO_PORTM_ICR_R = 0x2;
		SysTick_Wait10ms(BUTTON_DEBOUNCING); 
		HandleButton2Press();
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
	EnableInt();
	
	while (1) {
		StateMachine();
	}
}
