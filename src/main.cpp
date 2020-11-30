/*
 * main.cpp
 *
 *  Created on: Mar 30, 2019
 *      Author: Albert Guan
 */
 
#include <CameraMotor.h>
#include <Diag.h>
#include <iostream>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <wiringPi.h>

#include "DMA.h"
#include "GpioPwm.h"
#include "WS2812BCtrl.h"
#include "MotorCtrl.h"
#include "GpioClk.h"
#include "GpioIn.h"
#include "bcm2835.h"
#include "ProximitySensor.h"
#include "Diag.h"

int
main (
	_In_ int Argc,
	_In_ char *Argv[]
)

/*
 Routine Description:

	This routine is the entry point of the program.

 Parameters:

 	Argc - Supplies count of arguments.

 	Argv - Supplies argument values.

 Return Value:

	int - Not used, always 0

 */

{

	RPI_PRINT(InfoLevelInfo, "Hello Raspberry Pi!");

//	rpiI2CInit();

//	RBGControl();

//	dma_main();

//	PWMTest();

	WaterLight();

//	MotorDemo();

//	TwoMotorCtrl();

//	BuzzerTest();

//	JoyStickDemo();

//	ProximitySensorTest();
	return 0;
}


