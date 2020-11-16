/*
 * main.cpp
 *
 *  Created on: Mar 30, 2019
 *      Author: aobog
 */
#include <iostream>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <wiringPi.h>

#include "CameraMotors.h"
#include "DMA.h"
#include "GpioPwm.h"
#include "WS2812BCtrl.h"
#include "MotorCtrl.h"
#include "GpioClk.h"
#include "GpioIn.h"
#include "bcm2835.h"
#include "ProximitySensor.h"

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
	using namespace std;
	cout << "Hello Raspberry Pi!" << endl;

//	rpiI2CInit();

//	RBGControl();

//	dma_main();

//	PWMTest();

//	WS2812BCtrl leds(0.3);
//	leds.WaterLight();

//	MotorDemo();

//	TwoMotorCtrl();

//	BuzzerTest();

	JoyStickDemo();

//	ProximitySensorTest();
	return 0;
}


