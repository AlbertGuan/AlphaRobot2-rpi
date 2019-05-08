/*
 * PWM.cpp
 *
 *  Created on: Apr 23, 2019
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

#include "PWMBase.h"
#include "utilities.h"

volatile pwm_ctrl_t *PWMCtrl::pwm_base = NULL;
volatile uint32_t *PWMCtrl::clk_base = NULL;
volatile uint32_t *PWMCtrl::CM_PERIICTL = NULL;
volatile uint32_t *PWMCtrl::CM_PERIIDIV = NULL;

int32_t PWMCtrl::num_of_pwm_inst = 0;
int32_t PWMCtrl::pwm_1_in_use = -1;
int32_t PWMCtrl::pwm_2_in_use = -1;

PinSel_t PWMCtrl::getPinSel(uint32_t pin)
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

void PWMCtrl::Init()
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

			clk_base = const_cast<volatile uint32_t *>(static_cast<uint32_t *>(mmap(NULL, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, GPIO_CLK_PHY_ADDR)));
			if (MAP_FAILED == clk_base)
			{
				throw "Failed to mmap clk_base";
			}

		}
		catch (const std::string &str)
		{
			std::cout << "PWMCtrl Constructor got exception: " << str << std::endl;
		}
		CM_PERIICTL = const_cast<volatile uint32_t *>(reinterpret_cast<uint32_t *>((uint32_t) clk_base + CM_PERIICTL_OFFSET));
		CM_PERIIDIV = const_cast<volatile uint32_t *>(reinterpret_cast<uint32_t *>((uint32_t) clk_base + CM_PERIIDIV_OFFSET));
	}
}

void PWMCtrl::Uninit()
{
	if (0 == num_of_pwm_inst)
	{
		if (pwm_base)
		{
			munmap(static_cast<void *>(const_cast<pwm_ctrl_t *>(pwm_base)), sizeof(pwm_ctrl_t));
			pwm_base = NULL;
		}

		if (clk_base)
		{
			munmap(static_cast<void *>(const_cast<uint32_t *>(clk_base)), BLOCK_SIZE);
			clk_base = NULL;
			CM_PERIICTL = NULL;
			CM_PERIIDIV = NULL;
		}
	}
}

void PWMCtrl::getChannel()
{
	try
	{
		if (12 == m_pin
			|| 18 == m_pin
			|| 40 == m_pin
			|| 52 == m_pin)
		{
			if (pwm_1_in_use == -1)
			{
				m_pwm_channel = 1;
				pwm_1_in_use = m_pin;
			}
			else if (pwm_1_in_use != m_pin)
				throw "Failed to init pin " + std::to_string(m_pin) + " occupied by " + std::to_string(pwm_1_in_use);
		}
		else if (13 == m_pin
				|| 19 == m_pin
				|| 41 == m_pin
				|| 45 == m_pin
				|| 53 == m_pin)
		{
			if (pwm_2_in_use == -1)
			{
				m_pwm_channel = 2;
				pwm_2_in_use = m_pin;
			}
			else if (pwm_2_in_use != m_pin)
				throw "Failed to init pin " + std::to_string(m_pin) + " occupied by " + std::to_string(pwm_2_in_use);
		}
		else
			throw "Pin " + std::to_string(m_pin) + " doesn't support PWM";
	}
	catch (const std::string &exp)
	{
		std::cout << "PWMCtrl::getChannel() got exception: " << exp << std::endl;
	}
}

PWMCtrl::PWMCtrl(int32_t pin, int32_t range, int32_t divisor, int32_t mode, int32_t fifo)
	: GPIOBase(pin, getPinSel(pin))
{
	//Step 0: Init pointers to PWM and CLK registers
	PWMCtrl::Init();

	//Step 1: Figure out which PWM channel to use
	getChannel();

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

PWMCtrl::~PWMCtrl()
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
void PWMCtrl::SetClock(int32_t clk_div)
{
	uint32_t pwm_CTL = pwm_base->CTL.word;
	clk_div &= 4095;
	std::cout << "Set clock divisor to " << std::dec << clk_div << std::endl;
	//We need to stop the pwm and pwm clock before changing the clock divisor
	pwm_base->CTL.word = 0;
	//Some information on the CMPERIICTL: https://elinux.org/BCM2835_registers#CM_PERIICTL
	*CM_PERIICTL = BCM_PASSWORD | 0x01;
	usleep(1000);

	while ((*CM_PERIICTL & 0x80) != 0)	// Wait for clock to be !BUSY
		usleep(1);

	*CM_PERIIDIV = BCM_PASSWORD | (clk_div << 12);

	*CM_PERIICTL = BCM_PASSWORD | 0x11;	// Start PWM clock
	pwm_base->CTL.word = pwm_CTL;			// restore PWM_CONTROL
	usleep(1000);
}

void PWMCtrl::SetMode(uint32_t mode)
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

void PWMCtrl::SetRange(uint32_t range)
{
	if (1 == m_pwm_channel)
		pwm_base->RNG1 = range;
	else if (2 == m_pwm_channel)
		pwm_base->RNG2 = range;
	usleep(100);
}

void PWMCtrl::pwmWrite(uint32_t val)
{
	if (1 == m_pwm_channel)
		pwm_base->DAT1 = val;
	else if (2 == m_pwm_channel)
		pwm_base->DAT2 = val;
}

void PWMCtrl::pwmWriteFIFO(uint32_t *vals, uint32_t len)
{
	for (uint32_t i = 0; i < len; ++i)
	{
		pwm_base->FIF1 = vals[i];
	}
}

void PWMCtrl::SetPWMCtrl(const pwm_reg_CTL_t& ctl)
{
	pwm_base->CTL.word = ctl.word;
	usleep(100);
}

const volatile pwm_reg_CTL_t& PWMCtrl::GetPWMCTL()
{
	return pwm_base->CTL;
}

void PWMCtrl::PWMOnOff(int32_t val)
{
	assert(pwm_base != NULL);
	if (1 == m_pwm_channel)
		pwm_base->CTL.MSEN1 = val;
	else if (2 == m_pwm_channel)
		pwm_base->CTL.MSEN2 = val;
	else
		assert(false);
}

void PWMCtrl::PrintAddress()
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

//void PWMCtrl::DumpRegisters()
//{
//	using namespace std;
//	for (int i = 0; i < 6; ++i)
//		cout << "gpio_base->select[" << i << "]:\t\tAddr: 0x" << setw(8) << hex << (uint32_t) &gpio_base->select[i] << "\t Val: " << setw(8) << hex << gpio_base->select[i] << endl;
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
//	cout << "CM_PERIICTL:\t\tAddr: 0x" << setw(8) << hex << (uint32_t) CM_PERIICTL << "\t Val: " << setw(8) << hex << *CM_PERIICTL << endl;
//	cout << "CM_PERIIDIV:\t\tAddr: 0x" << setw(8) << hex << (uint32_t) CM_PERIIDIV << "\t Val: " << setw(8) << hex << *CM_PERIIDIV << endl;
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

void PWMCtrl::SetPWMCtrl(int32_t mode, int32_t fifo)
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

void PWMCtrl::ClearFIFO()
{
	pwm_base->CTL.CLRF1 = 1;
}

void PWMTest()
{
	//Note: In serializer mode, set the range to 32 since every word is only 32bits, or there will be gaps between FIFO words
	PWMCtrl pwm(18, 32, 8, 1, 1);
	pwm.ClearFIFO();

	uint32_t vals[10];

	float brightness = 1;
	const uint32_t led_val = 60;
	LEDPixel_t leds[4] = {
			{led_val, 0 ,0},
			{0, led_val, 0},
			{0, 0, led_val},
			{led_val, led_val, led_val}
	};

	for (int i = 0; i < 10; ++i)
		vals[i] = 0;
	setSerializedRGB(vals, 0, { 0, 0, 0 }, brightness);
	setSerializedRGB(vals, 1, { 0, 0, 0 }, brightness);
	setSerializedRGB(vals, 2, { 0, 0, 0 }, brightness);
	setSerializedRGB(vals, 3, { 0, 0, 0 }, brightness);

	int idx = 0;
	while (1)
	{
		++idx;
		setSerializedRGB(vals, 0, leds[idx % 4], brightness);
		setSerializedRGB(vals, 1, leds[(idx + 1) % 4], brightness);
		setSerializedRGB(vals, 2, leds[(idx + 2) % 4], brightness);
		setSerializedRGB(vals, 3, leds[(idx + 3) % 4], brightness);
		pwm.pwmWriteFIFO(vals, 9);
		pwm.PWMOnOff(ON);
		usleep(1000);

		pwm.PWMOnOff(OFF);
		pwm.ClearFIFO();
		sleep(1);
	}
}

