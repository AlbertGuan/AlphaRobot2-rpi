/*
 * GpioIn.h
 *
 *  Created on: May 11, 2019
 *      Author: Albert Guan
 */

#pragma once

#include "GpioBase.h"

class GpioIn : public GpioBase
{
public:
	typedef enum {
		NONE,
		InputRising,
		InputFalling,
		InputHigh,
		InputLow,
		InputAsyncRising,
		InputAsyncFalling,
	} GpioInEvent;

	GpioIn(int32_t pin, GpioInEvent event = NONE);
	virtual ~GpioIn();


	static void checkPinLevels(const std::vector<int32_t> &pins, std::vector<int32_t> &pin_levels);
	static void checkPinEvents(const std::vector<int32_t> &pins, std::vector<int32_t> &pin_events);
	int32_t getValue();
	int32_t checkEvent();
	void clearEventReg();
	int32_t operator[](const int32_t idx);
	const char *getEventName();
private:
	GpioInEvent m_event;
	uint32_t m_word_off;
	uint32_t m_mask;
};

void JoyStickDemo();

