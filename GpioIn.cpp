/*
 * GpioIn.cpp
 *
 *  Created on: May 11, 2019
 *      Author: aobog
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
	printf("GpioIn Constructor\n");

	m_word_off = (pin >= 32) ? 1 : 0;
	m_mask = 0x1u << (pin % 32);

	//Reset Event Detections
	gpio_base->async_rising_detect_enable[m_word_off] &= ~m_mask;
	usleep(100);
	gpio_base->async_falling_detect_enable[m_word_off] &= ~m_mask;
	usleep(100);
	gpio_base->rising_edge_detect_enable[m_word_off] &= ~m_mask;
	usleep(100);
	gpio_base->falling_edge_detect_enable[m_word_off] &= ~m_mask;
	usleep(100);
	gpio_base->high_detect_enable[m_word_off] &= ~m_mask;
	usleep(100);
	gpio_base->low_detect_enable[m_word_off] &= ~m_mask;
	usleep(100);
	//Reset Event Registers
	gpio_base->event_detect_status[m_word_off] |= m_mask;
	usleep(100);

	gpio_base->pull_enable = PULL_UP;	//Pull up
	usleep(100);
	gpio_base->pull_enable_clk[m_word_off] |= m_mask;
	usleep(100);
	gpio_base->pull_enable = 0;
	gpio_base->pull_enable_clk[m_word_off] &= ~m_mask;
	usleep(100);

	//Set Event Detection
	switch(event)
	{
		case AsyncRising:
			gpio_base->async_rising_detect_enable[m_word_off] |= m_mask;
			break;
		case AsyncFalling:
			gpio_base->async_falling_detect_enable[m_word_off] |= m_mask;
			break;
		case Rising:
			gpio_base->rising_edge_detect_enable[m_word_off] |= m_mask;
			break;
		case Falling:
			printf("Setting Falling edge 0x%08x\n", (uint32_t)&gpio_base->falling_edge_detect_enable[m_word_off] - (uint32_t)gpio_base);
			gpio_base->falling_edge_detect_enable[m_word_off] |= m_mask;
			break;
		case High:
			gpio_base->high_detect_enable[m_word_off] |= m_mask;
			break;
		case Low:
			gpio_base->low_detect_enable[m_word_off] |= m_mask;
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
	return this->m_pin[idx];
}

int32_t GpioIn::getValue()
{
	return (gpio_base->pin_level[m_word_off] & m_mask) ? 1 : 0;
}

int32_t GpioIn::checkEvent()
{
	return (gpio_base->event_detect_status[m_word_off] & m_mask) ? 1 : 0;
}

void GpioIn::clearEventReg()
{
	gpio_base->event_detect_status[m_word_off] |= m_mask;
}

const char *GpioIn::getEventName()
{
	switch(m_event)
	{
		case NONE:
			return "NONE";
		case Rising:
			return "Rising";
		case Falling:
			return "Falling";
		case High:
			return "High";
		case Low:
			return "Low";
		case AsyncRising:
			return "AsyncRising";
		case AsyncFalling:
			return "AsyncFalling";
		default:
			return "Unknown";
	}
}

void GpioIn::checkPinLevels(const std::vector<int32_t> &pins, std::vector<int32_t> &pin_levels)
{
	uint32_t words_to_check[2];

	words_to_check[0] = gpio_base->pin_level[0];
	words_to_check[1] = gpio_base->pin_level[1];

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

	words_to_check[0] = gpio_base->event_detect_status[0];
	words_to_check[1] = gpio_base->event_detect_status[1];

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
	GpioIn::GpioInEvent event = GpioIn::AsyncRising;
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

