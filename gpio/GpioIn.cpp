/*
 * GpioIn.cpp
 *
 *  Created on: May 11, 2019
 *      Author: Albert Guan
 */
#include <iostream>
#include <vector>
#include <iomanip>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <bitset>
#include <assert.h>
#include <exception>
#include "GpioIn.h"

/*
 * Note: Two lessons learned on GPIO input
 * 1. Don't enable too many event detections at the same time, or it will hang
 * 2. Pull up the pin
 * */
GpioIn::GpioIn(int32_t pin, GpioInEvent event)
	: GpioBase(std::vector<int32_t>{pin}, FSEL_INPUT),
	  m_event(event)
{
	std::cout << "GpioIn Constructor" << std::endl;

	m_word_off = (pin >= 32) ? 1 : 0;
	m_mask = 0x1u << (pin % 32);

	//Reset Event Detections
	GPIORegs->GPARENn[m_word_off] &= ~m_mask;
	usleep(100);
	GPIORegs->GPAFENn[m_word_off] &= ~m_mask;
	usleep(100);
	GPIORegs->GPRENn[m_word_off] &= ~m_mask;
	usleep(100);
	GPIORegs->GPFENn[m_word_off] &= ~m_mask;
	usleep(100);
	GPIORegs->GPHENn[m_word_off] &= ~m_mask;
	usleep(100);
	GPIORegs->GPLENn[m_word_off] &= ~m_mask;
	usleep(100);
	//Reset Event Registers
	GPIORegs->GPEDSn[m_word_off] |= m_mask;
	usleep(100);

	GPIORegs->GPPUD = PULL_UP;	//Pull up
	usleep(100);
	GPIORegs->GPPUDCLKn[m_word_off] |= m_mask;
	usleep(100);
	GPIORegs->GPPUD = 0;
	GPIORegs->GPPUDCLKn[m_word_off] &= ~m_mask;
	usleep(100);

	//Set Event Detection
	switch(event)
	{
		case InputAsyncRising:
			GPIORegs->GPARENn[m_word_off] |= m_mask;
			break;
		case InputAsyncFalling:
			GPIORegs->GPAFENn[m_word_off] |= m_mask;
			break;
		case InputRising:
			GPIORegs->GPRENn[m_word_off] |= m_mask;
			break;
		case InputFalling:
			printf("Setting Falling edge 0x%08x\n", (uint32_t)&GPIORegs->GPFENn[m_word_off] - (uint32_t)GPIORegs);
			GPIORegs->GPFENn[m_word_off] |= m_mask;
			break;
		case InputHigh:
			GPIORegs->GPHENn[m_word_off] |= m_mask;
			break;
		case InputLow:
			GPIORegs->GPLENn[m_word_off] |= m_mask;
			break;
		default:
			break;
	}

	usleep(100);

}

GpioIn::~GpioIn()
{
}


int32_t GpioIn::operator[](const int32_t idx)
{
	return this->m_Pins[idx];
}

int32_t GpioIn::getValue()
{
	return (GPIORegs->GPLEVn[m_word_off] & m_mask) ? 1 : 0;
}

int32_t GpioIn::checkEvent()
{
	return (GPIORegs->GPEDSn[m_word_off] & m_mask) ? 1 : 0;
}

void GpioIn::clearEventReg()
{
	GPIORegs->GPEDSn[m_word_off] |= m_mask;
}

const char *GpioIn::getEventName()
{
	switch(m_event)
	{
		case NONE:
			return "NONE";
		case InputRising:
			return "Rising";
		case InputFalling:
			return "Falling";
		case InputHigh:
			return "High";
		case InputLow:
			return "Low";
		case InputAsyncRising:
			return "AsyncRising";
		case InputAsyncFalling:
			return "AsyncFalling";
		default:
			return "Unknown";
	}
}

void GpioIn::checkPinLevels(const std::vector<int32_t> &pins, std::vector<int32_t> &pin_levels)
{
	uint32_t words_to_check[2];

	words_to_check[0] = GPIORegs->GPLEVn[0];
	words_to_check[1] = GPIORegs->GPLEVn[1];

	for (auto pin : pins)
	{
		uint32_t word_off = 0;
		if (pin >= 32)
		{
			pin %= 32;
			word_off = 1;
		}
		pin_levels.push_back(words_to_check[word_off] & (0x1u << pin));
	}
}

void GpioIn::checkPinEvents(const std::vector<int32_t> &pins, std::vector<int32_t> &pin_events)
{
	uint32_t words_to_check[2];

	words_to_check[0] = GPIORegs->GPEDSn[0];
	words_to_check[1] = GPIORegs->GPEDSn[1];

	for (auto pin : pins)
	{
		uint32_t word_off = 0;
		if (pin >= 32)
		{
			pin %= 32;
			word_off = 1;
		}
		pin_events.push_back(words_to_check[word_off] & (0x1u << pin));
	}
}

void JoyStickDemo()
{
	GpioIn::GpioInEvent event = GpioIn::InputAsyncRising;
	GpioIn front(8, event);
	GpioIn right(9, event);
	GpioIn left(10, event);
	GpioIn reverse(11, event);
	GpioIn center(7, event);

	std::vector<int32_t> pin_levels;
	std::vector<int32_t> pins {front[0], right[0], left[0], reverse[0], center[0]};
	std::vector<GpioIn *> pin_ptrs {&front, &right, &left, &reverse, &center};

	while (1)
	{
		GpioIn::checkPinLevels(pins, pin_levels);
		if (0 == pin_levels[0])
			printf("Front is low\n");
		if (0 == pin_levels[1])
			printf("Right is low\n");
		if (0 == pin_levels[2])
			printf("Left is low\n");
		if (0 == pin_levels[3])
			printf("Reverse is low\n");
		if (0 == pin_levels[4])
			printf("Center is low\n");
		pin_levels.clear();
		usleep(100000);
	}
//	while (1)
//	{
//		printf("Start checking events\n");
//
//		GpioIn::checkPinEvents(pins, pin_levels);
//		if (pin_levels[0])
//			printf("Front event %s happened\n", front.getEventName());
//		if (pin_levels[1])
//			printf("Right event %s happened\n", right.getEventName());
//		if (pin_levels[2])
//			printf("Left event %s happened\n", left.getEventName());
//		if (pin_levels[3])
//			printf("Reverse event %s happened\n", reverse.getEventName());
//		if (pin_levels[4])
//			printf("Center event %s happened\n", center.getEventName());
//		pin_levels.clear();
//		for (int i = 0; i < pin_ptrs.size(); ++i)
//		{
//			pin_ptrs[i]->checkEvent();
//		}
//		usleep(100000);
//	}
}

