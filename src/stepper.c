#include <stepper.h>


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