/*
 * GpioI2C.cpp
 *
 *  Created on: May 8, 2019
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
#include <assert.h>
#include <exception>
#include "GpioI2C.h"

const uint32_t GpioI2C::GPIO_I2C_PHY_ADDR[2] = { PERIPHERAL_PHY_BASE + GPIO_I2C0_OFFSET, PERIPHERAL_PHY_BASE + GPIO_I2C1_OFFSET};
int32_t GpioI2C::num_of_i2c_inst = 0;
int32_t GpioI2C::i2c_0_in_use = -1;
int32_t GpioI2C::i2c_1_in_use = -1;

PinSel_t GpioI2C::getPinSel(int32_t sda, int32_t scl)
{
	if (0 == sda && 1 == scl)
		return FSEL_ALT_0;
	else if (2 == sda && 3 == scl)
		return FSEL_ALT_0;
	else if (28 == sda && 29 == scl)
		return FSEL_ALT_0;
	//Note: We don't support using I2C0 on pin 44, 45
	else if (44 == sda && 45 == scl)
		return FSEL_ALT_2;
	else
	{
		std::cout << "SDA " << sda << " SCL " << scl << " doesn't support I2C!" << std::endl;
		assert(false);
	}
	return FSEL_INPUT;
}

void GpioI2C::getChannel()
{
	try
	{
		if ((0 == m_pin[0] && 1 == m_pin[1])
			|| (28 == m_pin[0] && 29 == m_pin[1]))
		{
			if (i2c_0_in_use == -1)
			{
				m_i2c_channel = 0;
				i2c_0_in_use = m_pin[0];
			}
			else if (i2c_0_in_use != m_pin[0])
				throw "Failed to init pin " + std::to_string(m_pin[0]) + " occupied by " + std::to_string(i2c_0_in_use);
		}
		else if ((2 == m_pin[0] && 3 == m_pin[1])
				|| (44 == m_pin[0] && 45 == m_pin[1]))
		{
			if (i2c_1_in_use == -1)
			{
				m_i2c_channel = 1;
				i2c_1_in_use = m_pin[0];
			}
			else if (i2c_1_in_use != m_pin[0])
				throw "Failed to init pin " + std::to_string(m_pin[0]) + " occupied by " + std::to_string(i2c_1_in_use);
		}
		else
			throw "Pin " + std::to_string(m_pin[0]) + " doesn't support I2C";
	}
	catch (const std::string &exp)
	{
		std::cout << "GpioPwm::getChannel() got exception: " << exp << std::endl;
	}
}

void GpioI2C::OnOff(int32_t val)
{
	m_i2c_base->C.I2CEN = val;
}

void GpioI2C::ClearFIFO()
{
	m_i2c_base->C.CLEAR = 1;
}


GpioI2C::GpioI2C(int32_t pin_sda, int32_t pin_scl)
	: GpioBase({pin_sda, pin_scl}, getPinSel(pin_sda, pin_scl))
{

	//Step 0: Get channel number
	getChannel();

	//Step 1: map registers
	try
	{
		m_i2c_base = const_cast<volatile I2CReg_t *>(reinterpret_cast<I2CReg_t *>(mmap(NULL, sizeof(I2CReg_t), PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, GPIO_I2C_PHY_ADDR[m_i2c_channel])));
		if (MAP_FAILED == m_i2c_base)
			throw "Failed to map m_i2c_base channel " + std::to_string(m_i2c_channel);
	}
	catch (const std::string &exp)
	{
		std::cout << "GpioI2C Constructor got exception: " << exp << std::endl;
	}

	//Step 2: Disable the channel
	OnOff(OFF);
	usleep(100);

	//Step 3: Clear the FIFO
	ClearFIFO();

}

GpioI2C::~GpioI2C()
{
	//Disable the I2C
	OnOff(OFF);

	//Clear the FIFO
	ClearFIFO();

	if (m_i2c_base)
	{
		munmap(static_cast<void *>(const_cast<I2CReg_t *>(m_i2c_base)), sizeof(I2CReg_t));
		m_i2c_base = NULL;
	}
}


