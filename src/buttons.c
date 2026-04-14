#include "buttons.h"
#include "stepper.h"

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