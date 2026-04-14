#include "isr.h"

void TIMER0A_IRQHandler(void) {
    TIMER0_ICR_R = 0x1;   // clear timeout flag
    motor_resume = 1;
}

void TIMER1A_IRQHandler(void) {
    TIMER1_ICR_R = 0x1;   // clear timeout flag
	turn_off_led3();
}

void SysTick_Handler(void) {
	ms_ticks++;
}

void GPIOJ_IRQHandler(void) {
	uint32_t mis = GPIO_PORTJ_MIS_R;
	GPIO_PORTJ_ICR_R = mis & 0x3;

	if (mis & 0x1  && ((GPIO_PORTJ_DATA_R & 0x1) == 0)) { // button0 pressed
		if (ms_ticks - last_time_buttton0_pressed >= BUTTON_DEBOUNCING) {
			last_time_buttton0_pressed = ms_ticks;
			button0_pressed = 1;
		} 
	} if (mis & 0x2  && ((GPIO_PORTJ_DATA_R & 0x2) == 0)) { // button1 pressed
		if (ms_ticks - last_time_buttton1_pressed >= BUTTON_DEBOUNCING) {
			last_time_buttton1_pressed = ms_ticks;
			button1_pressed = 1;
		}
	}
}

void GPIOM_IRQHandler(void) {
	uint32_t mis = GPIO_PORTM_MIS_R;
	GPIO_PORTM_ICR_R = 0x3;
	
	if ((mis & 0x1) && ((GPIO_PORTM_DATA_R & 0x1) == 0)) { // button2 pressed	
		if (ms_ticks - last_time_buttton2_pressed >= BUTTON_DEBOUNCING) {
			last_time_buttton2_pressed = ms_ticks;
			button2_pressed = 1;
		} 
	} if ((mis & 0x2) && ((GPIO_PORTM_DATA_R & 0x2) == 0)) { // button3 pressed
		if (ms_ticks - last_time_buttton3_pressed >= BUTTON_DEBOUNCING) {
			last_time_buttton3_pressed = ms_ticks;
			button3_pressed = 1;
		} 
	}
}
