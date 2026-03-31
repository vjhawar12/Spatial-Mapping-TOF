/*  

  Code written to support data collection from VL53L1X using the Ultra Light Driver.
  I2C methods written based upon MSP432E4 Reference Manual Chapter 19.
  Specific implementation was based upon format specified in VL53L1X.pdf pg19-21
  Code organized according to en.STSW-IMG009\Example\Src\main.c
  
  The VL53L1X is run with default firmware settings.
*/
#include <stdint.h>
#include "PLL.h"
#include "SysTick.h"
#include "uart.h"
#include "onboardLEDs.h"
#include "tm4c1294ncpdt.h"
#include "VL53L1X_api.h"


#define I2C_MCS_ACK             0x00000008  // Data Acknowledge Enable
#define I2C_MCS_DATACK          0x00000008  // Acknowledge Data
#define I2C_MCS_ADRACK          0x00000004  // Acknowledge Address
#define I2C_MCS_STOP            0x00000004  // Generate STOP
#define I2C_MCS_START           0x00000002  // Generate START
#define I2C_MCS_ERROR           0x00000002  // Error
#define I2C_MCS_RUN             0x00000001  // I2C Master Enable
#define I2C_MCS_BUSY            0x00000001  // I2C Busy
#define I2C_MCR_MFE             0x00000010  // I2C Master Function Enable

#define MAXRETRIES              5           // number of receive attempts before giving up


//XSHUT     This pin is an active-low shutdown input; 
//					the board pulls it up to VDD to enable the sensor by default. 
//					Driving this pin low puts the sensor into hardware standby. This input is not level-shifted.
void VL53L1X_XSHUT(void){
    GPIO_PORTG_DIR_R |= 0x01;                                        // make PG0 out
    GPIO_PORTG_DATA_R &= 0b11111110;                                 //PG0 = 0
    FlashAllLEDs();
    SysTick_Wait10ms(10);
    GPIO_PORTG_DIR_R &= ~0x01;                                            // make PG0 input (HiZ)
    
}


uint16_t	dev = 0x29;			//address of the ToF sensor as an I2C slave peripheral 0x29 before
int status=0;

uint16_t wordData;
uint16_t Distance;
uint16_t SignalRate;
uint16_t AmbientRate;
uint16_t SpadNum; 
uint8_t RangeStatus;
uint8_t dataReady;

void tof_init(void) {
    uint8_t byteData, sensorState = 0;
    uint16_t wordData;

    VL53L1X_XSHUT();          // force sensor reset
    SysTick_Wait10ms(10);     // extra settle time

    for (int tries = 0; tries < 50; tries++) {
        status = VL53L1X_BootState(dev, &sensorState);
        //UART_printf(printf_buffer);
        if (sensorState) break;
        SysTick_Wait10ms(10);
    }

    if (!sensorState) {
        //UART_printf("ERROR: ToF boot timeout\r\n");
        return;
    }

    //UART_printf("ToF Chip Booted!\r\n");
	
	status = VL53L1X_ClearInterrupt(dev); /* clear interrupt has to be called to enable next interrupt*/
	
  /* 2 Initialize the sensor with the default setting  */
  status = VL53L1X_SensorInit(dev);
	Status_Check("SensorInit", status);

    // 4 What is missing -- refer to API flow chart
  status = VL53L1X_StartRanging(dev);   // This function has to be called to enable the ranging

	
  /* 3 Optional functions to be used to change the main ranging parameters according the application requirements to get the best ranging performances */
//  status = VL53L1X_SetDistanceMode(dev, 2); /* 1=short, 2=long */
//  status = VL53L1X_SetTimingBudgetInMs(dev, 100); /* in ms possible values [20, 50, 100, 200, 500] */
//  status = VL53L1X_SetInterMeasurementInMs(dev, 200); /* in ms, IM must be > = TB */

  status = VL53L1_RdByte(dev, 0x010F, &byteData); //for model ID (0xEA)
  //sprintf(printf_buffer, "Model ID: 0x%X\r\n", byteData);
  //UART_printf(printf_buffer);

  status = VL53L1_RdByte(dev, 0x0110, &byteData); //for module type (0xCC)
  //sprintf(printf_buffer, "Module type: 0x%X\r\n", byteData);
  //UART_printf(printf_buffer);

  status = VL53L1_RdWord(dev, 0x010F, &wordData); //for both model ID and type
  //sprintf(printf_buffer, "Model ID & module type: 0x%X\r\n", wordData);
  //UART_printf(printf_buffer);
}

void tof_get_distance_nonblocking(void) {
  status = VL53L1X_GetDistance(dev, &Distance) ;					//The Measured Distance value
  status = VL53L1X_ClearInterrupt(dev); /* 8 clear interrupt has to be called to enable next interrupt*/
}

void tof_get_distance(void) {
  // Get the Distance Measures 50 times
  dataReady = 0;
  // 5 wait until the ToF sensor's data is ready
  while (dataReady == 0){
    status = VL53L1X_CheckForDataReady(dev, &dataReady);
    VL53L1_WaitMs(dev, 3);
  }
  dataReady = 0;
  
  status = VL53L1X_GetDistance(dev, &Distance) ;					//The Measured Distance value
  status = VL53L1X_ClearInterrupt(dev); /* 8 clear interrupt has to be called to enable next interrupt*/
}

void tof_scan(void) {
	// Get the Distance Measures 50 times
	for(int i = 0; i < 50; i++) {
		dataReady = 0;
		// 5 wait until the ToF sensor's data is ready
	  while (dataReady == 0){
		      status = VL53L1X_CheckForDataReady(dev, &dataReady);
          FlashLED3(1);
          VL53L1_WaitMs(dev, 3);
	  }
		dataReady = 0;
	  
		//7 read the data values from ToF sensor
		status = VL53L1X_GetRangeStatus(dev, &RangeStatus);
	  status = VL53L1X_GetDistance(dev, &Distance) ;					//The Measured Distance value
		status = VL53L1X_GetSignalRate(dev, &SignalRate);
		status = VL53L1X_GetAmbientRate(dev, &AmbientRate);
		status = VL53L1X_GetSpadNb(dev, &SpadNum);
    
		FlashLED4(1);

	  status = VL53L1X_ClearInterrupt(dev); /* 8 clear interrupt has to be called to enable next interrupt*/
		
		// print the resulted readings to UART
		//sprintf(printf_buffer,"%u, %u, %u, %u, %u\r\n", RangeStatus, Distance, SignalRate, AmbientRate,SpadNum);
		//UART_printf(printf_buffer);
	  SysTick_Wait10ms(50);
  }
  
	VL53L1X_StopRanging(dev);
  while(1) {}
}

void tof_stop(void) {
  	VL53L1X_StopRanging(dev);
}
