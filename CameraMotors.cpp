/*
 * I2C.cpp
 *
 *  Created on: Apr 9, 2019
 *      Author: aobog
 */
#include "CameraMotors.h"

#include <iostream>
#include <unistd.h>

/*Based on the datasheet of SG90 motor
 *1. the PWM period is 20ms (50Hz)
 *2. the range of duty cycle is 1-2ms, the range of PCA9685 is 0-4096 which maps to 0-100% duty cycle,
 *   so the range we could use is 204-409 (5-10%)
 */
#define LEFT_RIGHT_SERVO		0
#define LEFT_RIGTH_MIN			60
#define LEFT_RIGTH_RANGE		150

#define UP_DOWM_SERVO			1
#define UP_DOWN_MIN				100
#define UP_DOWN_RANGE			60


#define PCA9685_I2C_ADDRESS		0x40
#define SERVO_MOTOR_PWM_FREQ	50

//The osc of PCA9685 is 25MHz
#define PCA9685_OSC_FREQ		25000000.0f
//Register addresses of PCA9685
#define REG_MODE1_ADDR					0x00
#define REG_MODE2_ADDR					0x01

#define REG_LEDx_ON_LOW_ADDR(x)			(0x06 + ((x) << 2))
#define REG_LEDx_ON_HIGH_ADDR(x)		(0x07 + ((x) << 2))
#define REG_LEDx_OFF_LOW_ADDR(x)		(0x08 + ((x) << 2))
#define REG_LEDx_OFF_HIGH_ADDR(x)		(0x09 + ((x) << 2))

#define REG_LEDALL_ON_LOW_ADDR			0xFA
#define REG_LEDALL_ON_HIGH_ADDR			0xFB
#define REG_LEDALL_OFF_LOW_ADDR			0xFC
#define REG_LEDALL_OFF_HIGH_ADDR		0xFD
#define REG_PRE_SCALE_ADDR				0xFE

void PCA9685PWMFreq(int fd, int freq)
{
	if (freq > 1000)
	{
		std::cout << "Maxium freq is 1000" << std::endl;
		freq = 1000;
	}
	else if (freq < 40)
	{
		std::cout << "Minimum freq is 40" << std::endl;
		freq = 40;
	}

	//Calculate the prescal value, refer to PRE_SCALE (page 25 of PCA9685 datasheet) for more details
	int prescal =(int)(PCA9685_OSC_FREQ / (4096 * freq) + 0.5);

	//According to the foot note at page 13, "Writes to PRE_SCALE register are blocked when SLEEP bit is logic 0 (MODE1)"
	//1. Set the sleep bit
	//2. Set the PRE_SCALE
	//3. Clear the sleep bit
	unsigned char mode1_val = (unsigned char)wiringPiI2CReadReg8(fd, REG_MODE1_ADDR);
	wiringPiI2CWriteReg8(fd, REG_MODE1_ADDR, mode1_val | 0x10);		//Bit 4 of MODE1 register, 1: lower power mode, 0: normal mode
	wiringPiI2CWriteReg8(fd, REG_PRE_SCALE_ADDR, prescal);
	wiringPiI2CWriteReg8(fd, REG_MODE1_ADDR, mode1_val & (~0x10));

	//According to foot node 2 at page 14, "It takes 500us max for the oscillator to be up and running once SLEEP bit has been
	//set to logic 0"
	usleep(1000);
	//Restart all PWM channels, chapter 7.3.1.1 for more details
	wiringPiI2CWriteReg8(fd, REG_MODE1_ADDR, (mode1_val | 0x80) & (~0x10));
}

void PCA9685Control(int fd, int pin, int val)
{
	wiringPiI2CWriteReg16(fd, REG_LEDx_ON_LOW_ADDR(pin), 0x0);
	wiringPiI2CWriteReg16(fd, REG_LEDx_OFF_LOW_ADDR(pin), val & 0x0FFF);
}

void rpiI2CInit()
{
	using namespace std;
	int fd;
	if (-1 == wiringPiSetup())
	{
		cout << "Failed to setup wiringPi!" << endl;
		return;
	}

	fd = wiringPiI2CSetup(PCA9685_I2C_ADDRESS);

	//Enable the auto-increment of registers
	int mode1_val = wiringPiI2CReadReg8(fd, REG_MODE1_ADDR);
	wiringPiI2CWriteReg8(fd, REG_MODE1_ADDR, mode1_val | 0x20);

	PCA9685PWMFreq(fd, SERVO_MOTOR_PWM_FREQ);

	//Reset PWM outputs
	wiringPiI2CWriteReg16(fd, REG_LEDALL_ON_LOW_ADDR, 0x0);
	wiringPiI2CWriteReg16(fd, REG_LEDALL_OFF_LOW_ADDR, 0x1000);
	sleep(1);
	int lr_val = LEFT_RIGTH_MIN;
	int ud_val = UP_DOWN_MIN;
	int percentage = 0;
	int increase = 1;
	while(1)
	{
		PCA9685Control(fd, LEFT_RIGHT_SERVO, lr_val);
		PCA9685Control(fd, UP_DOWM_SERVO, ud_val);
		usleep(500000);
		if (increase)
			percentage += 2;
		else
			percentage -= 2;
		if (percentage >= 100)
			increase = 0;
		else if (percentage <= 0)
			increase = 1;
		lr_val = LEFT_RIGTH_MIN + (LEFT_RIGTH_RANGE * percentage) / 100;
		ud_val = UP_DOWN_MIN + (UP_DOWN_RANGE * percentage) / 100;
	}
}

