#include "onboardLEDs.h"

//Flash D1
void FlashLED1(int count) {
		while(count--) {
			GPIO_PORTN_DATA_R ^= 0b00000010; 								//hello world!
			SysTick_Wait10ms(DELAY);														//.05s delay
			GPIO_PORTN_DATA_R ^= 0b00000010;			
			SysTick_Wait10ms(DELAY);														//.05s delay
		}
}
//Flash D2
void FlashLED2(int count) {
		while(count--) {
			GPIO_PORTN_DATA_R ^= 0b00000001; 								//hello world!
			SysTick_Wait10ms(DELAY);														//.05s delay
			GPIO_PORTN_DATA_R ^= 0b00000001;			
			SysTick_Wait10ms(DELAY);														//.05s delay			
		}
}
//Flash D3
void FlashLED3(int count) {
		while(count--) {
			GPIO_PORTF_DATA_R ^= 0b00010000; 								//hello world!
			SysTick_Wait10ms(DELAY);														//.05s delay
			GPIO_PORTF_DATA_R ^= 0b00010000;			
			SysTick_Wait10ms(DELAY);														//.05s delay			
		}
}
//Flash D4
void FlashLED4(int count) {
		while(count--) {
			GPIO_PORTF_DATA_R ^= 0b00000001; 								//hello world!
			SysTick_Wait10ms(DELAY);														//.05s delay
			GPIO_PORTF_DATA_R ^= 0b00000001;			
			SysTick_Wait10ms(DELAY);														//.05s delay			
		}
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

void flash_led3() {
	GPIO_PORTF_DATA_R |= 0x1; // turning on
					//scan(); LED
	TIMER1_CTL_R &= ~0x1; // ensure timer is off
	TIMER1_ICR_R = 0x1;       // clear timeout flag
	TIMER1_TAILR_R = LED_BLINK_DELAY;
	TIMER1_CTL_R |= 0x1; // turn timer on again
}

void turn_on_led2() {
	GPIO_PORTF_DATA_R |= 0x10;
}

void turn_off_led2() {
	GPIO_PORTF_DATA_R &= ~0x10;
}

void FlashAllLEDs(){
		GPIO_PORTN_DATA_R ^= 0b00000011; 								//hello world!
		GPIO_PORTF_DATA_R ^= 0b00010001; 								//hello world!	
		SysTick_Wait10ms(25);														//.25s delay
		GPIO_PORTN_DATA_R ^= 0b00000011;			
		GPIO_PORTF_DATA_R ^= 0b00010001; 								//goodbye world!	
		SysTick_Wait10ms(25);														//.25s delay			
}

void FlashI2CTx(){
//	FlashLED1(1);
}

void FlashI2CRx(){
//	FlashLED2(1);
}

//Flash Error D1&D2&D3&D4
void FlashI2CError(int count) {
//		while(count--) {
//			FlashAllLEDs();
//		}
}

// Initialize onboard LEDs
void onboardLEDs_Init(void){
	//Use PortN onboard LEDs	
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R12;				// activate clock for Port N
	while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R12) == 0){};	// allow time for clock to stabilize
	GPIO_PORTN_DIR_R |= 0x03;        								// make PN0 out (PN0 built-in LED1)
  	GPIO_PORTN_AFSEL_R &= ~0x03;     								// disable alt funct on PN0,PN1
  	GPIO_PORTN_DEN_R |= 0x03;        								// enable digital I/O on PN0,PN1
																								// configure PN1,PN0 as GPIO
  	//GPIO_PORTN_PCTL_R = (GPIO_PORTN_PCTL_R&0xFFFFFF00)+0x00000000;
  	GPIO_PORTN_AMSEL_R &= ~0x03;     								// disable analog functionality on PN0,PN1

	//Use PortF onboard LEDs	
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R5;				// activate clock for Port N
	while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R5) == 0){};	// allow time for clock to stabilize
	GPIO_PORTF_DIR_R |= 0x11;        								// make PN4,0 out 
  	GPIO_PORTF_AFSEL_R &= ~0x11;     								// disable alt funct on PF4,PF0
  	GPIO_PORTF_DEN_R |= 0x11;        								// enable digital I/O on PF4,PF0
																									// configure PF4,PF0 as GPIO
  	//GPIO_PORTN_PCTL_R = (GPIO_PORTN_PCTL_R&0xFFF0FFF0)+0x00000000;
  	GPIO_PORTF_AMSEL_R &= ~0x011;     								// disable analog functionality on PF4,PF0
		
	FlashAllLEDs();
	return;
}




