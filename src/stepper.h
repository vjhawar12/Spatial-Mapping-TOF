#include "constants.h"
#include <stdint.h>
#include "tm4c1294ncpdt.h"
#include "PLL.h"
#include "tof.h"
#include "uart.h"
#include "stdio.h"
#include "VL53L1X_api.h"
#include "buttons.h"

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
volatile int scan_pos = 0;
volatile int scan_move_steps = 0;
volatile uint32_t ms_ticks = 0;
volatile int motor_resume = 1;
volatile int scan_ready = 0;
volatile int scan_pending = 0;
extern uint16_t Distance;
extern uint16_t	dev;
extern int status;
int incr_dist[] = {0, 100, 200};
int i = 0;
int scan_done = 0;

void stop(void);
void full_step_once_forward(void);
void full_step_once_reverse(void);
void reverse_until_home(void);
void forward_until_home(void);
void home(void);