/*
 * PWM.cpp
 *
 *  Created on: Apr 23, 2019
 *      Author: Albert Guan
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
#include <Diag.h>
#include <exception>

#include "GpioPwm.h"

volatile GpioPwm::pwm_ctrl_t *GpioPwm::pwm_base = NULL;
volatile uint32_t *GpioPwm::ClkRegisters = NULL;
volatile uint32_t *GpioPwm::CM_PWMCTL = NULL;
volatile uint32_t *GpioPwm::CM_PWMDIV = NULL;

int32_t GpioPwm::num_of_pwm_inst = 0;
int32_t GpioPwm::pwm_1_in_use = -1;
int32_t GpioPwm::pwm_2_in_use = -1;

GPIO_FUN_SELECT
GpioPwm::GetChannelFromPin (
	uint32_t pin
)

{
	if (12 == pin)
		return FSEL_ALT_0;
	else if (13 == pin)
		return FSEL_ALT_0;
	else if (18 == pin)
		return FSEL_ALT_5;
	else if (19 == pin)
		return FSEL_ALT_5;
	else if (40 == pin)
		return FSEL_ALT_0;
	else if (41 == pin)
		return FSEL_ALT_0;
	else
	{
		std::cout << "Pin " << pin << " doesn't support PWM!" << std::endl;
		assert(false);
	}

	return FSEL_INPUT;
}

void GpioPwm::GetChannel()
{
	try
	{
		if (12 == m_Pins[0]
			|| 18 == m_Pins[0]
			|| 40 == m_Pins[0]
			|| 52 == m_Pins[0])
		{
			if (pwm_1_in_use == -1)
			{
				m_pwm_channel = 1;
				pwm_1_in_use = m_Pins[0];
			}
			else if (pwm_1_in_use != m_Pins[0])
				throw "Failed to init pin " + std::to_string(m_Pins[0]) + " occupied by " + std::to_string(pwm_1_in_use);
		}
		else if (13 == m_Pins[0]
				|| 19 == m_Pins[0]
				|| 41 == m_Pins[0]
				|| 45 == m_Pins[0]
				|| 53 == m_Pins[0])
		{
			if (pwm_2_in_use == -1)
			{
				m_pwm_channel = 2;
				pwm_2_in_use = m_Pins[0];
			}
			else if (pwm_2_in_use != m_Pins[0])
				throw "Failed to init pin " + std::to_string(m_Pins[0]) + " occupied by " + std::to_string(pwm_2_in_use);
		}
		else
			throw "Pin " + std::to_string(m_Pins[0]) + " doesn't support PWM";
	}
	catch (const std::string &exp)
	{
		std::cout << "GpioPwm::GetChannel() got exception: " << exp << std::endl;
	}
}

void GpioPwm::Init()
{
	if (0 == num_of_pwm_inst)
	{
		try
		{
			pwm_base = const_cast<volatile pwm_ctrl_t *>(static_cast<pwm_ctrl_t *>(mmap(NULL, sizeof(pwm_ctrl_t), PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, GPIO_PWM_PHY_ADDR)));
			if (MAP_FAILED == pwm_base)
			{
				throw "Failed to mmap pwm_base";
			}

			ClkRegisters = const_cast<volatile uint32_t *>(static_cast<uint32_t *>(mmap(NULL, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, GPIO_CLK_PHY_ADDR)));
			if (MAP_FAILED == ClkRegisters)
			{
				throw "Failed to mmap ClkRegisters";
			}

		}
		catch (const std::string &exp)
		{
			std::cout << "GpioPwm Constructor got exception: " << exp << std::endl;
		}
		CM_PWMCTL = const_cast<volatile uint32_t *>(reinterpret_cast<uint32_t *>((uint32_t) ClkRegisters + CM_PWMCTL_OFFSET));
		CM_PWMDIV = const_cast<volatile uint32_t *>(reinterpret_cast<uint32_t *>((uint32_t) ClkRegisters + CM_PWMDIV_OFFSET));
	}
}

void GpioPwm::Uninit()
{
	if (0 == num_of_pwm_inst)
	{
		if (pwm_base)
		{
			munmap(static_cast<void *>(const_cast<pwm_ctrl_t *>(pwm_base)), sizeof(pwm_ctrl_t));
			pwm_base = NULL;
		}

		if (ClkRegisters)
		{
			munmap(static_cast<void *>(const_cast<uint32_t *>(ClkRegisters)), BLOCK_SIZE);
			ClkRegisters = NULL;
			CM_PWMCTL = NULL;
			CM_PWMDIV = NULL;
		}
	}
}

GpioPwm::GpioPwm(int32_t pin, int32_t range, int32_t divisor, int32_t mode, int32_t fifo)
	: GpioBase({pin}, GetChannelFromPin(pin))
{
	//Step 0: Init pointers to PWM and CLK registers
	GpioPwm::Init();

	//Step 1: Figure out which PWM channel to use
	GetChannel();

	//Step 2: Turn off the PWM before making changes
	PWMOnOff(OFF);

	//Step 3: Setup CTL register
	SetPWMCtrl(mode, fifo);

	//Step 4: Setup PWM range
	SetRange(range);

	//Step 5: Setup PWM Clock
	SetClock(divisor);
	std::cout << "PWM frequency is: " << PWM_CLK_SRC_REQ / range / divisor << std::endl;

	//Step 6: Acknowledge the PWM BERR
	pwm_base->STA.BERR = 1;
	while (pwm_base->STA.BERR);

	++num_of_pwm_inst;
	//Note: The PWM hasn't been enabled yet!!!
}

GpioPwm::~GpioPwm()
{
	if (1 == m_pwm_channel)
	{
		pwm_base->CTL.PWEN1 = 0;
		pwm_1_in_use = -1;
	}
	else if (2 == m_pwm_channel)
	{
		pwm_base->CTL.PWEN2 = 0;
		pwm_2_in_use = -1;
	}

	--num_of_pwm_inst;
	Uninit();

}

/* Unfortunately, the description to clock manager of BCM2835/2827 is missing in the datasheet
 * I'm trying to "reverse-engineering" the wiringPi library along with information I can find
 * through Google.
 */
void GpioPwm::SetClock(int32_t clk_div)
{
	uint32_t pwm_CTL = pwm_base->CTL.word;
	clk_div &= 4095;
	std::cout << "Set clock divisor to " << std::dec << clk_div << std::endl;
	//We need to stop the pwm and pwm clock before changing the clock divisor
	pwm_base->CTL.word = 0;
	//Some information on the CMPERIICTL: https://elinux.org/BCM2835_registers#CM_PWMCTL
	*CM_PWMCTL = BCM_PASSWORD | 0x01;
	usleep(1000);

	while ((*CM_PWMCTL & 0x80) != 0)	// Wait for clock to be !BUSY
		usleep(1);

	*CM_PWMDIV = BCM_PASSWORD | (clk_div << 12);

	*CM_PWMCTL = BCM_PASSWORD | 0x11;	// Start PWM clock
	pwm_base->CTL.word = pwm_CTL;			// restore PWM_CONTROL
	usleep(1000);
}

void GpioPwm::SetMode(uint32_t mode)
{
	pwm_reg_CTL_t temp;
	if (1 == m_pwm_channel)
	{
		temp.word = pwm_base->CTL.word;
		temp.MSEN1 = mode;
		pwm_base->CTL.word = temp.word;
	}
	else if (2 == m_pwm_channel)
	{
		temp.word = pwm_base->CTL.word;
		temp.MSEN2 = mode;
		pwm_base->CTL.word = temp.word;
	}
	usleep(100);
}

void GpioPwm::SetRange(uint32_t range)
{
	if (1 == m_pwm_channel)
		pwm_base->RNG1 = range;
	else if (2 == m_pwm_channel)
		pwm_base->RNG2 = range;
	usleep(100);
}

void GpioPwm::pwmWrite(uint32_t val)
{
	if (1 == m_pwm_channel)
		pwm_base->DAT1 = val;
	else if (2 == m_pwm_channel)
		pwm_base->DAT2 = val;
}

void GpioPwm::pwmWriteFIFO(uint32_t *vals, uint32_t len)
{
	for (uint32_t i = 0; i < len; ++i)
	{
		pwm_base->FIF1 = vals[i];
	}
}

void GpioPwm::SetPWMCtrl(const pwm_reg_CTL_t& ctl)
{
	pwm_base->CTL.word = ctl.word;
	usleep(100);
}

const volatile GpioPwm::pwm_reg_CTL_t& GpioPwm::GetPWMCTL()
{
	return pwm_base->CTL;
}

void GpioPwm::PWMOnOff(int32_t val)
{
	assert(pwm_base != NULL);
	if (1 == m_pwm_channel)
		pwm_base->CTL.PWEN1 = val;
	else if (2 == m_pwm_channel)
		pwm_base->CTL.PWEN2 = val;
	else
		assert(false);
}

void GpioPwm::PrintAddress()
{
	using namespace std;
	cout << "pwm_base->CTL:\t\t0x" << setw(8) << hex << (uint32_t) &pwm_base->CTL << endl;
	cout << "pwm_base->STA:\t\t0x" << setw(8) << hex << (uint32_t) &pwm_base->STA << endl;
	cout << "pwm_base->DMAC:\t\t0x" << setw(8) << hex << (uint32_t) &pwm_base->DMAC << endl;
	cout << "pwm_base->RNG1:\t\t0x" << setw(8) << hex << (uint32_t) &pwm_base->RNG1 << endl;
	cout << "pwm_base->DAT1:\t\t0x" << setw(8) << hex << (uint32_t) &pwm_base->DAT1 << endl;
	cout << "pwm_base->FIF1:\t\t0x" << setw(8) << hex << (uint32_t) &pwm_base->FIF1 << endl;
	cout << "pwm_base->RNG2:\t\t0x" << setw(8) << hex << (uint32_t) &pwm_base->RNG2 << endl;
	cout << "pwm_base->DAT2:\t\t0x" << setw(8) << hex << (uint32_t) &pwm_base->DAT2 << endl;
}

//void GpioPwm::DumpRegisters()
//{
//	using namespace std;
//	for (int i = 0; i < 6; ++i)
//		cout << "GPIORegs->GPFSELn[" << i << "]:\t\tAddr: 0x" << setw(8) << hex << (uint32_t) &GPIORegs->GPFSELn[i] << "\t Val: " << setw(8) << hex << GPIORegs->GPFSELn[i] << endl;
//	cout << endl;
//
//	cout << "pwm_base->CTL:\t\tAddr: 0x" << setw(8) << hex << (uint32_t) &pwm_base->CTL << "\t Val: " << setw(8) << hex << pwm_base->CTL.word << endl;
//	cout << "pwm_base->STA:\t\tAddr: 0x" << setw(8) << hex << (uint32_t) &pwm_base->STA << "\t Val: " << setw(8) << hex << pwm_base->STA.word << endl;
//	cout << "pwm_base->DMAC:\t\tAddr: 0x" << setw(8) << hex << (uint32_t) &pwm_base->DMAC << "\t Val: " << setw(8) << hex << pwm_base->DMAC.word << endl;
//	cout << "pwm_base->RNG1:\t\tAddr: 0x" << setw(8) << hex << (uint32_t) &pwm_base->RNG1 << "\t Val: " << setw(8) << hex << pwm_base->RNG1 << endl;
//	cout << "pwm_base->DAT1:\t\tAddr: 0x" << setw(8) << hex << (uint32_t) &pwm_base->DAT1 << "\t Val: " << setw(8) << hex << pwm_base->DAT1 << endl;
//	cout << "pwm_base->FIF1:\t\tAddr: 0x" << setw(8) << hex << (uint32_t) &pwm_base->FIF1 << "\t Val: " << setw(8) << hex << pwm_base->FIF1 << endl;
//	cout << "pwm_base->RNG2:\t\tAddr: 0x" << setw(8) << hex << (uint32_t) &pwm_base->RNG2 << "\t Val: " << setw(8) << hex << pwm_base->RNG2 << endl;
//	cout << "pwm_base->DAT2:\t\tAddr: 0x" << setw(8) << hex << (uint32_t) &pwm_base->DAT2 << "\t Val: " << setw(8) << hex << pwm_base->DAT2 << endl;
//	cout << endl;
//
//	cout << "CM_PWMCTL:\t\tAddr: 0x" << setw(8) << hex << (uint32_t) CM_PWMCTL << "\t Val: " << setw(8) << hex << *CM_PWMCTL << endl;
//	cout << "CM_PWMDIV:\t\tAddr: 0x" << setw(8) << hex << (uint32_t) CM_PWMDIV << "\t Val: " << setw(8) << hex << *CM_PWMDIV << endl;
//}


typedef struct
{
	uint32_t R;
	uint32_t G;
	uint32_t B;
} LEDPixel_t;
//Each led requires 24 pixels, each pixel contains 3 bits in "arr"(0b110 for "1" and 0b100 for "0")
void setSerializedRGB(uint32_t *arr, const int led_idx, const LEDPixel_t &color, const float brightness = 1.0)
{
	uint32_t R = color.R * brightness;
	uint32_t G = color.G * brightness;
	uint32_t B = color.B * brightness;
	uint32_t color_comp = (G << 16) | (R << 8) | B;
//	printf("led[%d]: color_comp: 0x%08x\n", led_idx, color_comp);
	uint32_t mask = 0x1;
	for (int pixel = 0; pixel < 24; ++pixel, mask <<= 1)
	{
		int32_t word_off = (led_idx * 72 + pixel * 3) / 32;
		int32_t bit_off = 29 - (led_idx * 72 + pixel * 3) % 32;
		if (bit_off >= 0)
		{
			arr[word_off] &= ~(0x7 << bit_off);
			if (color_comp & mask)
				arr[word_off] |= 0x6 << bit_off;
			else
				arr[word_off] |= 0x4 << bit_off;
		}
		else if (bit_off == -1)
		{
			arr[word_off] &= ~(0x3);
			arr[word_off + 1] &= ~(0x1 << (32 + bit_off));
			if (color_comp & mask)
				arr[word_off] |= 0x3;
			else
				arr[word_off] |= 0x2;
		}
		else if (bit_off == -2)
		{
			arr[word_off] |= 0x1;
			arr[word_off + 1] &= ~(0x3 << (32 + bit_off));
			if (color_comp & mask)
				arr[word_off + 1] |= 0x2 << (32 + bit_off);
		}
	}
}

void GpioPwm::SetPWMCtrl(int32_t mode, int32_t fifo)
{
	pwm_reg_CTL_t pwm_ctl;
	pwm_ctl.word = GetPWMCTL().word;
	if (1 == m_pwm_channel)
	{
		pwm_ctl.word &= 0xFFFFFF00;
		//Set to serialiser mode
		pwm_ctl.MODE1 = mode;
		//Use the FIFO
		pwm_ctl.USEF1 = fifo;
	}
	SetPWMCtrl(pwm_ctl);
}

void GpioPwm::ClearFIFO()
{
	pwm_base->CTL.CLRF1 = 1;
}

