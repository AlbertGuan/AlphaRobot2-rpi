/*
 * GpioIn.h
 *
 *  Created on: May 11, 2019
 *      Author: aobog
 */

#pragma once

#include "GpioBase.h"

class GpioIn : public GpioBase
{
public:
	GpioIn(int32_t pin);
	virtual ~GpioIn();
	typedef enum
	{
		Rising,
		Falling,
		High,
		Low,
		AsyncRising,
		AsyncFalling,
	}GpioInEvent;
	static void CheckInput(const std::vector<int32_t> &candidates, std::vector<int32_t> &rise_pins, GpioInEvent event);

	int32_t operator[](const int32_t idx);
private:

};

void JoyStickDemo();

