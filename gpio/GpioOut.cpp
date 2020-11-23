/*
 * GpioOut.cpp
 *
 *  Created on: May 7, 2019
 *      Author: Albert Guan
 */
#include "GpioOut.h"

GpioOut::GpioOut(int32_t pin)
	: GpioBase({pin}, FSEL_OUTPUT)
{
}

GpioOut::~GpioOut()
{
}

void GpioOut::Update(const std::vector<uint32_t> &set_pins, const std::vector<uint32_t> &clear_pins)
{
	uint32_t set[2] = {0, 0};
	uint32_t clear[2] = {0, 0};
	for (auto pin : set_pins)
		set[pin >> 5] |= 0x1u << (pin - (pin >> 5 << 5));
	for (auto pin : clear_pins)
		clear[pin >> 5] |= 0x1u << (pin - (pin >> 5 << 5));

	for (int i = 0; i < 2; ++i)
	{
		if (set[i] != 0)
			GPIORegs->GPSETn[i] = set[i];
		if (clear[i] != 0)
			GPIORegs->GPCLRn[0] = clear[i];
	}
}



