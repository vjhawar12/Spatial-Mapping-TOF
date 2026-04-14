#include "tm4c1294ncpdt.h"
#include "buttons.h"
#include "stepper.h"

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
	stop();

	GPIO_PORTJ_ICR_R = 0x3;
	GPIO_PORTM_ICR_R = 0x3;
	EnableInt();

	button0_pressed = 0;
	button1_pressed = 0;
	button2_pressed = 0;
	button3_pressed = 0;
	motor_resume = 1;

	while (1) {
		StateMachine();
		if (button0_pressed) HandleButton0Press();
		if (button1_pressed) HandleButton1Press();
		if (button2_pressed) HandleButton2Press();
		if (button3_pressed) HandleButton3Press();
		
		if (scan_pending) {
			uint8_t ready = 0;
			status = VL53L1X_CheckForDataReady(dev, &ready);
			if (ready) {
				tof_get_distance_nonblocking();
				scan_pending = 0;
				print_scan(incr_dist[i]);
			}
		}

		if (scan_done) {
			i++; 
			if (i > 2) {
				UART_printf("ENDDATA\r\n"); 
				i = 0;
			}
			scan_done = 0;
		}
	}

}