/*
 * GPIO.cpp
 *
 *  Created on: May 7, 2019
 *      Author: aguan
 */
#include <iostream>
#include <iomanip>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <bitset>
#include <string>

#include "GpioBase.h"

volatile GpioBase::gpio_reg_t *GpioBase::gpio_base = NULL;
int32_t GpioBase::num_of_gpio_inst = 0;

GpioBase::GpioBase(const std::vector<int32_t> &pin, PinSel_t pin_sel)
	: m_pin(pin),
	  m_pin_sel(pin_sel)
{
	Init();

	SetPinSelection();
	++num_of_gpio_inst;
}

GpioBase::~GpioBase()
{
	--num_of_gpio_inst;
		Uninit();
}

void GpioBase::SetPinSelection()
{
	for (auto pin : m_pin)
	{
		uint32_t word_off = pin / 10;		//Each GPIO selection word contains 10 pins
		uint32_t bit_off = pin % 10 * 3;	//Each pin takes 3 bits in GPIO selection word

		uint32_t before = gpio_base->select[word_off];
		std::bitset<32> b(before);
		before &= ~(0x7 << bit_off);
		before |= m_pin_sel << bit_off;
		std::bitset<32> a(before);
		gpio_base->select[word_off] = before;
		std::cout << "Before: " << b << std::endl;
		std::cout << "After:  " << a << std::endl;
	}
}

int32_t GpioBase::Init()
{
	if (0 == num_of_gpio_inst)
	{
		//Check whether the "/dev/mem" has been opened or not
		if (mem_fd != 0)
		{
			try
			{
				gpio_base = (gpio_reg_t *) mmap(NULL, sizeof(gpio_reg_t), PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, GPIO_BASE_PHY_ADDR);
				if (MAP_FAILED == gpio_base)
					throw "Failed to mmap gpio_base";
			}
			catch (const std::string &err)
			{
				std::cout << __func__ << " with exception: " << err << std::endl;
				return -1;
			}
			catch (...)
			{
				std::cout << __func__ << " with unknown exception" << std::endl;
				return -2;
			}
		}
	}
	return 0;
}

void GpioBase::Uninit()
{
	if (0 == num_of_gpio_inst)
	{
		if (gpio_base)
		{
			//Only const_cast can add/remove the "volatile" keyword
			munmap(static_cast<void *>(const_cast<gpio_reg_t *>(gpio_base)), sizeof(gpio_reg_t));
			gpio_base = NULL;
		}
	}
}
