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
#include "bcm2835.h"

#include <wiringPi.h>

#include "CameraMotors.h"
#include "DMA.h"
#include "PWMBase.h"
#include "WS2812BCtrl.h"
#include "MotorCtrl.h"


int main(int argc, char *argv[])
{
	using namespace std;
	cout << "Hello Raspberry Pi!" << endl;

	//rpiI2CInit();

//	RBGControl();

//	dma_main();

//	PWMTest();

	WS2812BCtrl leds(0.3);
	leds.WaterLight();

//	MotorDemo();
	return 0;
}


