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

GpioIn::GpioIn(int32_t pin)
	: GpioBase({pin}, FSEL_INPUT)
{
	uint32_t word_off = pin >= 32 ? 1 : 0;
	uint32_t mask = pin - (pin >> 5 << 5);
	gpio_base->async_falling_detect_enable[word_off] = mask;
	gpio_base->async_rising_detect_enable[word_off] = mask;
	gpio_base->falling_edge_detect_enable[word_off] = mask;
	gpio_base->rising_edge_detect_enable[word_off] = mask;
	gpio_base->high_detect_enable[word_off] = mask;
	gpio_base->low_detect_enable[word_off] = mask;

	std::cout << "gpio_base->event_detect_status[0]: " << gpio_base->event_detect_status[0] << std::endl;
	std::cout << "gpio_base->event_detect_status[1]: " << gpio_base->event_detect_status[1] << std::endl;
}

GpioIn::~GpioIn()
{
}

void GpioIn::CheckInput(const std::vector<int32_t> &candidates, std::vector<int32_t> &rise_pins, GpioInEvent event)
{
	uint32_t vals[2];
	for (int i = 0; i < 2; ++i)
	{
		switch(event)
		{
			case Rising:
				vals[i] = gpio_base->rising_edge_detect_enable[i];
				std::cout << "Rising: " << vals[i] << std::endl;
				break;
			case Falling:
				vals[i] = gpio_base->falling_edge_detect_enable[i];
				std::cout << "Falling: " << vals[i] << std::endl;
				break;
			case High:
				vals[i] = gpio_base->high_detect_enable[i];
				std::cout << "High: " << vals[i] << std::endl;
				break;
			case Low:
				vals[i] = gpio_base->low_detect_enable[i];
				std::cout << "Low: " << vals[i] << std::endl;
				break;
			case AsyncRising:
				vals[i] = gpio_base->async_rising_detect_enable[i];
				std::cout << "AsyncRising: " << vals[i] << std::endl;
				break;
			case AsyncFalling:
				vals[i] = gpio_base->async_falling_detect_enable[i];
				std::cout << "AsyncFalling: " << vals[i] << std::endl;
				break;
			default:
				return;
		}
	}

	for (auto pin : candidates)
	{
		uint32_t word_off = pin / 32;
		uint32_t mask = 0x1u << (pin - (pin >> 5 << 5));
		if (vals[word_off] & mask)
			rise_pins.push_back(pin);
	}
}

int32_t GpioIn::operator[](const int32_t idx)
{
	return this->m_pin[idx];
}

void JoyStickDemo()
{
	GpioIn front(8);
	GpioIn right(9);
	GpioIn left(10);
	GpioIn reverse(11);
	GpioIn center(7);

	std::vector<int32_t> rise_pins;
	while (1)
	{
		GpioIn::CheckInput(std::vector<int32_t>{front[0], right[0], left[0], reverse[0], center[0]}, rise_pins, GpioIn::Rising);
//		std::cout << "Rising pins: " << rise_pins.size() << std::endl;
		rise_pins.clear();

		GpioIn::CheckInput(std::vector<int32_t>{front[0], right[0], left[0], reverse[0], center[0]}, rise_pins, GpioIn::Falling);
//		std::cout << "Falling pins: " << rise_pins.size() << std::endl;
		rise_pins.clear();

		GpioIn::CheckInput(std::vector<int32_t>{front[0], right[0], left[0], reverse[0], center[0]}, rise_pins, GpioIn::High);
//		std::cout << "High pins: " << rise_pins.size() << std::endl;
		rise_pins.clear();

		GpioIn::CheckInput(std::vector<int32_t>{front[0], right[0], left[0], reverse[0], center[0]}, rise_pins, GpioIn::Low);
//		std::cout << "Low pins: " << rise_pins.size() << std::endl;
		rise_pins.clear();

		GpioIn::CheckInput(std::vector<int32_t>{front[0], right[0], left[0], reverse[0], center[0]}, rise_pins, GpioIn::AsyncRising);
//		std::cout << "AsyncRising pins: " << rise_pins.size() << std::endl;
		rise_pins.clear();

		GpioIn::CheckInput(std::vector<int32_t>{front[0], right[0], left[0], reverse[0], center[0]}, rise_pins, GpioIn::AsyncFalling);
//		std::cout << "AsyncFalling pins: " << rise_pins.size() << std::endl;
		rise_pins.clear();

		sleep(1);
	}
}

