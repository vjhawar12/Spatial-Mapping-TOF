#include "stateMachine.h"

void print_scan(int x) {
	int absolute_angle =  (scan_pos * 36000) / STEPS_PER_REV;
	sprintf(printf_buffer, "%d,%3d.%02d,%u\r\n", x, absolute_angle / 100, absolute_angle % 100, Distance);
	UART_printf(printf_buffer);
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
				if (move_steps == STEPS_PER_REV) {
					state = STOP;
					scan_done = 1;
				}
				if ((move_steps % blink_div == 0) && !scan_pending) {
					flash_led3();
					scan_pending = 1;
					scan_pos = pos;	
					scan_move_steps = move_steps;
				} 
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
				if (move_steps == STEPS_PER_REV) {
					state = STOP;
					scan_done = 1;
				}
				if (move_steps % blink_div == 0) {
					flash_led3();
					scan_pending = 1;
					scan_pos = pos;	
					scan_move_steps = move_steps;
				} 
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
