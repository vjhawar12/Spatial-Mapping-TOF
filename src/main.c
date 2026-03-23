#include <stdint.h>
#include "tm4c1294ncpdt.h"
#include "SysTick.h"
#include "PLL.h"
#include "tof.h"
#include "uart.h"
#include "stdio.h"

#define STEPS_PER_REV 2048
#define STEP_11_25 64
#define STEP_45  256

// 3.5 ms delay
#define DELAY 440000

// 20 ms delay
#define LED_BLINK_DELAY 2500000

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

// setting this to 3 required for forward operation at start
int pattern_index = 3;

// volatile because they're modified in ISRs and read outside of them
volatile int blink_div = STEP_11_25;
volatile int dir = 1; // 1: forward, 0: reverse

volatile int pos = 0; // just in case
volatile int move_steps = 0;
volatile int button0_pressed = 0;
volatile int button1_pressed = 0;
volatile int button2_pressed = 0;
volatile int button3_pressed = 0;

volatile int motor_resume = 1;
volatile int led_resume = 1;


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

void Timer0_Init() {
	// need a periodic timer
	// Ensure the timer is disabled (the TnEN bit in the GPTMCTL register is cleared) before making any changes.
	// Write the GPTM Configuration Register (GPTMCFG) with a value of 0x0000.0000.
	//Configure the TnMR field in the GPTM Timer n Mode Register (GPTMTnMR) with 0x2 for periodic
	// Load the start value into the GPTM Timer n Interval Load Register (GPTMTnILR)
	// set the appropriate bits in the GPTM Interrupt Mask Register
	// Set the TnEN bit in the GPTMCTL register to enable the timer and start counting.
	// status flags are cleared by writing a 1 to the appropriate bit of the GPTM Interrupt Clear Register (GPTMICR).
	SYSCTL_RCGCTIMER_R |= 0x1;
	TIMER0_ICR_R = 0x1;
	TIMER0_CTL_R &= ~0x1;
	TIMER0_CFG_R = 0x0;
	TIMER0_TAMR_R = 0x2;
	TIMER0_TAILR_R = DELAY;
	TIMER0_IMR_R |= 0x1;
	TIMER0_CTL_R |= 0x1;

	// enabling in NVIC, IRQ = 19
	NVIC_EN0_R |= (1 << 19);
}

void Timer1_Init() {
	// need a periodic timer
	// Ensure the timer is disabled (the TnEN bit in the GPTMCTL register is cleared) before making any changes.
	// Write the GPTM Configuration Register (GPTMCFG) with a value of 0x0000.0000.
	//Configure the TnMR field in the GPTM Timer n Mode Register (GPTMTnMR) with 0x2 for periodic
	// Load the start value into the GPTM Timer n Interval Load Register (GPTMTnILR)
	// set the appropriate bits in the GPTM Interrupt Mask Register
	// Set the TnEN bit in the GPTMCTL register to enable the timer and start counting.
	// status flags are cleared by writing a 1 to the appropriate bit of the GPTM Interrupt Clear Register (GPTMICR).
	SYSCTL_RCGCTIMER_R |= 0x2;
	TIMER1_ICR_R = 0x1;
	TIMER1_CTL_R &= ~0x1;
	TIMER1_CFG_R = 0x0;
	TIMER1_TAMR_R = 0x2;
	TIMER1_TAILR_R = LED_BLINK_DELAY;
	TIMER1_IMR_R |= 0x1;
	TIMER1_CTL_R |= 0x1;

	// enabling in NVIC, IRQ = 21
	NVIC_EN0_R |= (1 << 21);
}

void Timer1_pause() {
	TIMER1_CTL_R &= ~0x1;
}

void Timer1_resume() {
	TIMER1_CTL_R |= 0x1;
}


void TIMER0A_IRQHandler(void) {
    TIMER0_ICR_R = 0x1;   // clear timeout flag
    motor_resume = 1;
}


void TIMER1A_IRQHandler(void) {
    TIMER1_ICR_R = 0x1;   // clear timeout flag
    led_resume = 1;
	Timer1_pause();
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

void scan() {
	int distance = tof_get_distance();
	int relative_angle =  (move_steps * 36000) / STEPS_PER_REV;
	int absolute_angle =  (pos * 36000) / STEPS_PER_REV;

	sprintf(printf_buffer, "Relative Angle: %3d.%02d°\r\n Absolute Angle: %3d.%02d°\r\n Distance: %4d mm\r\n\r\n", relative_angle / 100, relative_angle % 100, absolute_angle / 100, absolute_angle % 100, distance);
	UART_printf(printf_buffer);
}


void reverse_until_home() {
	while (pos != 0) {
		if (motor_resume) {
			motor_resume = 0;
			full_step_once_reverse();
		}
	}
}

void forward_until_home() {
	while (pos != 0) {
		if (motor_resume) {
			motor_resume = 0;
			full_step_once_forward();
		}	
	}
}

void home() {
	int reverse_distance = pos - 0;
	int forward_distance = STEPS_PER_REV - pos;
	
	reverse_distance < forward_distance? reverse_until_home() : forward_until_home();
	state = STOP;
}

// called in while(true) loop 
void StateMachine() {
	switch(state) {
		case STOP:
			stop();
			turn_off_led0();
			turn_off_led2();
			turn_off_led3();
			move_steps = 0;
			break;
		case FORWARD:
			turn_on_led0();
			turn_on_led1();

			blink_div == STEP_11_25? turn_on_led2() : turn_off_led2();
			if (motor_resume) {
				motor_resume = 0;
				full_step_once_forward();
				move_steps++;

				if (led_resume) {
					Timer1_pause();
					led_resume = 0;
					turn_off_led3();
				}

				if (move_steps % blink_div == 0) {
					turn_on_led3();
					led_resume = 0;	
					scan();
				} 
			} 
			
			if (move_steps == STEPS_PER_REV) {
				state = STOP;
			}
			break;
		case REVERSE:
			turn_on_led0();
			turn_off_led1();

			blink_div == STEP_11_25? turn_on_led2() : turn_off_led2();

			if (motor_resume) {
				motor_resume = 0;

				full_step_once_reverse();
				move_steps++;

				if (led_resume) {
					Timer1_resume();
					led_resume = 0;
					turn_off_led3();
				}

				if (move_steps % blink_div == 0) {
					turn_on_led3();
					led_resume = 0;
					scan();
				} 
			}	

			if (move_steps == STEPS_PER_REV) {
				state = STOP;
			}
			break;
		case HOME:
			turn_off_led0();
			turn_off_led1();
			turn_off_led2();
			turn_off_led3();
			home();
			stop();
			break;
	}
}

// interrupt-based approach will make button0 call this upon press
void HandleButton0Press() {
	button0_pressed = 0;
	if (state == STOP) {
		state = dir? FORWARD : REVERSE;
	} else {
		state = STOP;
	}
}

// interrupt-based approach will make button1 call this upon press
void HandleButton1Press() {
	button1_pressed = 0;
	dir = !dir;
	move_steps = 0;
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
	button2_pressed = 0;
	if (blink_div == STEP_11_25) {
		blink_div = STEP_45;
	} else {
		blink_div = STEP_11_25;
	}
}

// interrupt-based approach will make button3 call this upon press
void HandleButton3Press() {
	button3_pressed = 0;
	state = HOME;
}

void GPIOJ_IRQHandler(void) {
	uint32_t mis = GPIO_PORTJ_MIS_R;
	GPIO_PORTJ_ICR_R = mis & 0x3;
	SysTick_Wait10ms(2); // button debouncing
	if (mis & 0x1  && ((GPIO_PORTJ_DATA_R & 0x1) == 0)) { // button0 pressed
		button0_pressed = 1;
	} if (mis & 0x2  && ((GPIO_PORTJ_DATA_R & 0x2) == 0)) { // button1 pressed
		button1_pressed = 1;
	}
}

void GPIOM_IRQHandler(void) {
	uint32_t mis = GPIO_PORTM_MIS_R;
	GPIO_PORTM_ICR_R = 0x3;
	SysTick_Wait10ms(BUTTON_DEBOUNCING); 
	
	if ((mis & 0x1) && ((GPIO_PORTM_DATA_R & 0x1) == 0)) { // button2 pressed	
		button2_pressed = 1;
	} else if ((mis & 0x2) && ((GPIO_PORTM_DATA_R & 0x2) == 0)) { // button3 pressed
		button3_pressed = 1;
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
	UART_Init();
	I2C_Init();
	Timer0_Init();
	Timer1_Init();

	tof_init();
	SysTick_Wait10ms(100);
	stop();

	GPIO_PORTJ_ICR_R = 0x3;
	GPIO_PORTM_ICR_R = 0x3;
	EnableInt();

	button0_pressed = 0;
	button1_pressed = 0;
	button2_pressed = 0;
	button3_pressed = 0;
	motor_resume = 1;
	led_resume = 1;

	while (1) {
		StateMachine();
		if (button0_pressed) HandleButton0Press();
		if (button1_pressed) HandleButton1Press();
		if (button2_pressed) HandleButton2Press();
		if (button3_pressed) HandleButton3Press();
	}

}
