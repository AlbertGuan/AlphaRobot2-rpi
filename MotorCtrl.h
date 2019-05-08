/*
 * MotorCtrl.h
 *
 *  Created on: May 5, 2019
 *      Author: aobog
 */
#pragma once
#include <stdint.h>
#include <stddef.h>     /* offsetof */
#include <vector>

#include "GPIOBase.h"
#include "Rpi3BConstants.h"
#include "GPIOOutput.h"

//Using the TB6612FNG H-bridge to control two motors
class MotorCtrl
{
public:
	MotorCtrl();
	virtual ~MotorCtrl();

	void ShortBrake();
	void CCW();
	void CW();
	void Stop();
private:
	const static uint32_t GPIO_AIN1 = 12;
	const static uint32_t GPIO_AIN2 = 13;
	const static uint32_t GPIO_PWMA = 6;
	const static uint32_t GPIO_BIN1 = 20;
	const static uint32_t GPIO_BIN2 = 21;
	const static uint32_t GPIO_PWMB = 26;

	std::vector<GPIOBase *> m_pins;
};

void MotorDemo();

