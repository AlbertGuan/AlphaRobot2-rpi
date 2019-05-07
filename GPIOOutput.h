/*
 * GPIOOut.h
 *
 *  Created on: May 7, 2019
 *      Author: aguan
 */

#pragma once
#include <vector>
#include "GPIOBase.h"
class GPIOOutput : public GPIOBase
{
public:
	GPIOOutput(uint32_t pin);
	virtual ~GPIOOutput();
	static void Update(const std::vector<uint32_t> &set_pins, const std::vector<uint32_t> &clear_pins);
protected:

};
