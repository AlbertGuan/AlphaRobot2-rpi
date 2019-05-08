/*
 * GPIOOut.h
 *
 *  Created on: May 7, 2019
 *      Author: aguan
 */

#pragma once
#include <vector>

#include "GpioBase.h"
class GpioOut : public GpioBase
{
public:
	GpioOut(int32_t pin);
	virtual ~GpioOut();
	static void Update(const std::vector<uint32_t> &set_pins, const std::vector<uint32_t> &clear_pins);
protected:

};
