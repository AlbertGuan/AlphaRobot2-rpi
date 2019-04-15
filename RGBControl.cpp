/*
 * RGBControl.cpp
 *
 *  Created on: Apr 14, 2019
 *      Author: aobog
 */
#include "RGBControl.h"
#include <iostream>
#include <unistd.h>

void TwoServoMotor()
{
	//Up and down: 100 - 160
#define PWM_PIN_UP_DOWN			1
	//Left and right: 60 - 210
#define PWM_PIN_LEFT_RIGHT		24
	pinMode(PWM_PIN_UP_DOWN, PWM_OUTPUT);
	pinMode(PWM_PIN_LEFT_RIGHT, PWM_OUTPUT);
	pwmSetMode(PWM_MODE_MS);
	pwmSetRange(2000);
	pwmSetClock(192);

	unsigned int val = 0;
	int inc = 1;
	while (1)
	{
		pwmWrite(PWM_PIN_UP_DOWN, 100 + val);
		pwmWrite(PWM_PIN_LEFT_RIGHT, 60 + val * 2);
		usleep(100000ULL);
		if (inc)
		{
			++val;
			if (val >= 60)
			{
				val = 60;
				inc = 0;
			}
		}
		else
		{
			--val;
			if (val <= 0)
			{
				val = 0;
				inc = 1;
			}
		}
	}
}


/*
 * The clock frequency of PWM is 19.2MHz, and the sum of on/off is 1.25us (800KHz)
 * So the clock divisor is 19.2M / 800K = 24
 *
 * To represent a logical 0, the high voltage time is 0.4us, low voltage time is 0.85 (32% duty cycle)
 * To represent a logical 1, the high voltage time is 0.8us, low voltage time is 0.45 (64% duty cycle)
 * In order to achieve these resolution, we select the PWM range as 100
  */
#define RGB_PWM_RANGE			100
#define RGB_PWM_CLK_DIV			24
#define RGB_PIN					1
#define RGB_LOGIC_0				32
#define RGB_LOGIC_1				64
void RBGControl()
{
	using namespace std;
	cout << "RBGControl" << endl;
	if (-1 == wiringPiSetup())
	{
		cout << "Failed to setup wiringPi!" << endl;
		return;
	}

	pinMode(RGB_PIN, PWM_OUTPUT);
	pwmSetMode(PWM_MODE_MS);
	pwmSetRange(RGB_PWM_RANGE);
	pwmSetClock(RGB_PWM_CLK_DIV);

	unsigned int val = 0;
	unsigned int counter = 0;
	while (1)
	{
		//if (counter < 24)
		{
			if (counter & 0x1)
				val = RGB_LOGIC_1;
			else
				val = RGB_LOGIC_0;
		}
		//else
		//	val = 0;
		pwmWrite(RGB_PIN, val);
		++counter;
	}
}
