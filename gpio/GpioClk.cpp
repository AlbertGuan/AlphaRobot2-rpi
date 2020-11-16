/*
 * GpioClk.cpp
 *
 *  Created on: May 11, 2019
 *      Author: aobog
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
#include "GpioClk.h"

const uint32_t GpioClk::CM_GPxCTL_OFFSET[3] = { 0x70, 0x78, 0x80 };			//CM_PWMCTL
const uint32_t GpioClk::CM_GPxDIV_OFFSET[3] = { 0x74, 0x7C, 0x84 };			//CM_PWMDIV
volatile uint32_t *GpioClk::clk_base = NULL;
int32_t GpioClk::num_of_clk_inst = 0;

GpioClk::GpioClk(int32_t pin, float freq)
	: GpioBase({pin}, FSEL_ALT_0),
	  m_Pins(pin)
{
	Init();

	m_channel = GetChannel(pin);

	CM_GP_CTL = const_cast<volatile uint32_t *>(reinterpret_cast<uint32_t *>((uint32_t) clk_base + CM_GPxCTL_OFFSET[m_channel]));
	CM_GP_DIV = const_cast<volatile uint32_t *>(reinterpret_cast<uint32_t *>((uint32_t) clk_base + CM_GPxDIV_OFFSET[m_channel]));

	SetFreq(freq);

	++num_of_clk_inst;
}

void GpioClk::SetFreq(float freq)
{
	assert(freq >= 0.0 && freq <= RPI_OSCILLATOR_FREQ);
	m_freq = freq;
	uint32_t divi = RPI_OSCILLATOR_FREQ / freq;
	uint32_t divr = RPI_OSCILLATOR_FREQ % static_cast<uint32_t>(freq);
	uint32_t divf = static_cast<uint32_t>(static_cast<float>(divr) * 4096.0 / RPI_OSCILLATOR_FREQ);

	std::cout << "SetFreq: freq: " << freq << std::endl;
	if (divi > 4095)
		divi = 4095;

	//Stop the clk
	Stop();

	//Set dividers
	*CM_GP_DIV = BCM_PASSWORD | (divi << 12) | divf;

	//Start the clock
	Start();
}
GpioClk::~GpioClk()
{
	Stop();
	--num_of_clk_inst;
	Uninit();
}

void GpioClk::Start()
{
	//Start the clock
	*CM_GP_CTL = BCM_PASSWORD | 0x10 | 0x1;
}

void GpioClk::Stop()
{
	//Using the oscillator as the clock source and stop the clock
	*CM_GP_CTL = BCM_PASSWORD | 0x1;
	while (*CM_GP_CTL & 0x80);
}

int32_t GpioClk::GetChannel(int32_t pin)
{
	switch(pin)
	{
		case 4:
			return 0;
		case 5:
			return 1;
		case 6:
			return 2;
		default:
			std::cout << "GpioClk: unsupported pin " << pin << std::endl;
	}
	return 0;
}

void GpioClk::Init()
{
	if (0 == num_of_clk_inst)
	{
		try
		{
			clk_base = const_cast<volatile uint32_t *>(static_cast<uint32_t *>(mmap(NULL, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, GPIO_CLK_PHY_ADDR)));
			if (MAP_FAILED == clk_base)
			{
				throw "Failed to mmap clk_base";
			}

		}
		catch (const std::string &exp)
		{
			std::cout << "GpioPwm Constructor got exception: " << exp << std::endl;
		}
	}
}

void GpioClk::Uninit()
{
	if (0 == num_of_clk_inst)
	{
		if (clk_base)
		{
			munmap(static_cast<void *>(const_cast<uint32_t *>(clk_base)), BLOCK_SIZE);
			clk_base = NULL;
		}
	}
}

void BuzzerTest()
{
	GpioClk buzzer(4, 200);
	float freq = 200.0;
	int incre = 1000;
	while (1)
	{
		buzzer.SetFreq(freq);
		freq += incre;
		if (freq >= 10000)
			incre = -1000;
		else if (freq < 1000)
			incre = 1000;
		usleep(100000);
	}
}
