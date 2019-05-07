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

#include "GPIOBase.h"


volatile gpio_reg_t *GPIOBase::gpio_base = NULL;
int32_t GPIOBase::num_of_gpio_inst = 0;

GPIOBase::GPIOBase(uint32_t pin, enum GPIO_FUN_SELECT alt)
	: m_pin(pin),
	  m_sel(alt)
{
	if (0 == num_of_gpio_inst)
		Init();
	++num_of_gpio_inst;
}

GPIOBase::~GPIOBase()
{
	--num_of_gpio_inst;
	if (0 == num_of_gpio_inst)
	{
		Uninit();
	}
}

void GPIOBase::SetPinSelection(uint32_t pin, enum GPIO_FUN_SELECT alt)
{
	uint32_t word_off = pin / 10;		//Each GPIO selection word contains 10 pins
	uint32_t bit_off = pin % 10 * 3;	//Each pin takes 3 bits in GPIO selection word
	printf("Pin %u: word_off: %u, bit_off: %u\n", pin, word_off, bit_off);
	uint32_t before = gpio_base->select[word_off];
	std::bitset<32> b(before);
	gpio_base->select[word_off] &= ~(0x7 << bit_off);
	gpio_base->select[word_off] |= alt << bit_off;
	std::bitset<32> a(gpio_base->select[word_off]);
	std::cout << "Before: " << b << std::endl;
	std::cout << "After:  " << a << std::endl;
}

int32_t GPIOBase::Init()
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
	return 0;
}

void GPIOBase::Uninit()
{
	if (gpio_base)
	{
		//Only const_cast can add/remove the "volatile" keyword
		munmap(static_cast<void *>(const_cast<gpio_reg_t *>(gpio_base)), sizeof(gpio_reg_t));
		gpio_base = NULL;
	}
}
