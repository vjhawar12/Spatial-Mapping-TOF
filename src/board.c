#include <stdint.h>
#include "tm4c1294ncpdt.h"
#include "constants.h"

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
	//    GPIO_PORTB_AMSEL_R &= ~0x0C;          															// 7) disable analog functionality on PB2,3
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
// motor timer
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

// led timer
void Timer1_Init() {
	// should be one-shot
	SYSCTL_RCGCTIMER_R |= 0x2;
	TIMER1_ICR_R = 0x1;
	TIMER1_CTL_R &= ~0x1;
	TIMER1_CFG_R = 0x0;
	TIMER1_TAMR_R = 0x1; // oneshot
	TIMER1_TAILR_R = LED_BLINK_DELAY;
	TIMER1_IMR_R |= 0x1;
	// enabling in NVIC, IRQ = 21
	NVIC_EN0_R |= (1 << 21);
}

static void SysTick_Init() {
	/* 
		. Program the value in the STRELOAD register.
2. Clear the STCURRENT register by writing to it with any value.
3. Configure the STCTRL register for the required operation
	*/
	NVIC_ST_CTRL_R &= ~NVIC_ST_CTRL_INTEN; // disabling interrupts
	NVIC_ST_RELOAD_R = SYSTICK_1MS_RELOAD; // setting reload value
	NVIC_ST_CURRENT_R = 0x0; // clearing STCURRENT
	NVIC_ST_CTRL_R |= NVIC_ST_CTRL_ENABLE; // enabling SysTick
	NVIC_ST_CTRL_R |= NVIC_ST_CTRL_INTEN; // enabling interrupts
}
