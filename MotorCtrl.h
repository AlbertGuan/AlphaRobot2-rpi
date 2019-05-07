/*
 * MotorCtrl.h
 *
 *  Created on: May 5, 2019
 *      Author: aobog
 */
#pragma once
#include <stdint.h>
#include <stddef.h>     /* offsetof */
#include "Rpi3BConstants.h"
#include "GPIO.h"

//Using the TB6612FNG H-bridge to control two motors
class MotorCtrl
{
public:
	MotorCtrl();
	virtual ~MotorCtrl() {}

	void SetPinSelection(uint32_t pin, enum GPIO_FUN_SELECT alt);
	void ShortBrake();
	void CCW();
	void CW();
	void Stop();
	static void AddrInit();
private:
	const static uint32_t GPIO_AIN1 = 12;
	const static uint32_t GPIO_AIN2 = 13;
	const static uint32_t GPIO_PWMA = 6;
	const static uint32_t GPIO_BIN1 = 20;
	const static uint32_t GPIO_BIN2 = 21;
	const static uint32_t GPIO_PWMB = 26;

	static const uint32_t GPIO_BASE_PHY_ADDR 	= PERIPHERAL_PHY_BASE + GPIO_BASE_OFFSET;

	static int32_t mem_fd;
	static volatile gpio_reg_t *gpio_base;
};

void MotorDemo();

