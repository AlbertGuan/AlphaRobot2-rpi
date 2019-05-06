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

#include "PWM.h"
#include "utilities.h"

int32_t PWMCtrl::mem_fd = -1;

volatile gpio_reg_t *PWMCtrl::gpio_base = NULL;
volatile pwm_ctrl_t *PWMCtrl::pwm_base = NULL;
volatile uint32_t *PWMCtrl::clk_base = NULL;
volatile uint32_t *PWMCtrl::CM_PERIICTL = NULL;
volatile uint32_t *PWMCtrl::CM_PERIIDIV = NULL;

int32_t PWMCtrl::pwm_1_in_use = -1;
int32_t PWMCtrl::pwm_2_in_use = -1;

void PWMCtrl::PWMAddrInit()
{
	using namespace std;
	if (mem_fd != 0)
	{
		if ((mem_fd = open("/dev/mem", O_RDWR | O_SYNC | O_CLOEXEC)) < 0)
		{
			//Error Handler
			std::cout << "PWMCtrl Constructor: Failed to open /dev/mem" << std::endl;
			return;
		}

		if (MAP_FAILED == (gpio_base = (gpio_reg_t *) mmap(NULL, sizeof(gpio_reg_t), PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, GPIO_BASE_PHY_ADDR)))
		{
			//Error Handler
			std::cout << "PWMCtrl Constructor: Failed to mmap gpio_base" << std::endl;
		}

		if (MAP_FAILED == (pwm_base = (pwm_ctrl_t *) mmap(NULL, sizeof(pwm_ctrl_t), PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, GPIO_PWM_PHY_ADDR)))
		{
			//Error Handler
			std::cout << "PWMCtrl Constructor: Failed to mmap pwm_base" << std::endl;
		}

		if (MAP_FAILED == (clk_base = (uint32_t *) mmap(NULL, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, GPIO_CLK_PHY_ADDR)))
		{
			//Error Handler
			std::cout << "PWMCtrl Constructor: Failed to mmap clk_base" << std::endl;
		}
		CM_PERIICTL = (uint32_t *) ((uint32_t) clk_base + CM_PERIICTL_OFFSET);
		CM_PERIIDIV = (uint32_t *) ((uint32_t) clk_base + CM_PERIIDIV_OFFSET);
	}

	//PrintAddress();

//	DumpRegisters();
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

void PWMCtrl::DumpRegisters()
{
	using namespace std;
	for (int i = 0; i < 6; ++i)
		cout << "gpio_base->select[" << i << "]:\t\tAddr: 0x" << setw(8) << hex << (uint32_t) &gpio_base->select[i] << "\t Val: " << setw(8) << hex << gpio_base->select[i] << endl;
	cout << endl;

	cout << "pwm_base->CTL:\t\tAddr: 0x" << setw(8) << hex << (uint32_t) &pwm_base->CTL << "\t Val: " << setw(8) << hex << pwm_base->CTL.word << endl;
	cout << "pwm_base->STA:\t\tAddr: 0x" << setw(8) << hex << (uint32_t) &pwm_base->STA << "\t Val: " << setw(8) << hex << pwm_base->STA.word << endl;
	cout << "pwm_base->DMAC:\t\tAddr: 0x" << setw(8) << hex << (uint32_t) &pwm_base->DMAC << "\t Val: " << setw(8) << hex << pwm_base->DMAC.word << endl;
	cout << "pwm_base->RNG1:\t\tAddr: 0x" << setw(8) << hex << (uint32_t) &pwm_base->RNG1 << "\t Val: " << setw(8) << hex << pwm_base->RNG1 << endl;
	cout << "pwm_base->DAT1:\t\tAddr: 0x" << setw(8) << hex << (uint32_t) &pwm_base->DAT1 << "\t Val: " << setw(8) << hex << pwm_base->DAT1 << endl;
	cout << "pwm_base->FIF1:\t\tAddr: 0x" << setw(8) << hex << (uint32_t) &pwm_base->FIF1 << "\t Val: " << setw(8) << hex << pwm_base->FIF1 << endl;
	cout << "pwm_base->RNG2:\t\tAddr: 0x" << setw(8) << hex << (uint32_t) &pwm_base->RNG2 << "\t Val: " << setw(8) << hex << pwm_base->RNG2 << endl;
	cout << "pwm_base->DAT2:\t\tAddr: 0x" << setw(8) << hex << (uint32_t) &pwm_base->DAT2 << "\t Val: " << setw(8) << hex << pwm_base->DAT2 << endl;
	cout << endl;

	cout << "CM_PERIICTL:\t\tAddr: 0x" << setw(8) << hex << (uint32_t) CM_PERIICTL << "\t Val: " << setw(8) << hex << *CM_PERIICTL << endl;
	cout << "CM_PERIIDIV:\t\tAddr: 0x" << setw(8) << hex << (uint32_t) CM_PERIIDIV << "\t Val: " << setw(8) << hex << *CM_PERIIDIV << endl;
}

PWMCtrl::PWMCtrl()
{
	PWMCtrl::PWMAddrInit();
}

PWMCtrl::PWMCtrl(int32_t pin, int32_t range, int32_t divisor)
{
	using namespace std;
	PWMCtrl::PWMAddrInit();

	if (divisor > 4095)
		cout << "Error: the divisor " << divisor << " is greater than 4095" << endl;

	switch (pin)
	{
		case 12:
			break;
		case 13:
			break;
		case 18:
		{
			if (-1 == pwm_1_in_use)
			{
				m_pwm_channel = 1;
				m_range = range;

				pwm_base->CTL.MSEN1 = 0;
				usleep(200);

				cout << "Setting pin 18 to PWM" << endl;
				cout << "pin selection[1] before changing: " << hex << gpio_base->select[1] << endl;
				//Note: Value of FSEL_ALT are not 0-5
				set_bits((uint32_t *) &gpio_base->select[1], FSEL_ALT_5, 26, 24);
				cout << "pin selection[1] after changing: " << hex << gpio_base->select[1] << endl;
				usleep(200);
				cout << "Set PWM range to " << dec << range << endl;
				SetRange(range);
				usleep(10);
				cout << "Set clock divisor to " << dec << divisor << endl;
				SetClock(divisor);
				cout << "PWM frequency is: " << PWMCtrl::PWM_CLK_SRC_REQ / range / divisor << endl;
				pwm_1_in_use = 18;
			}
			else
				cout << "PWM 1 is already in use on pin: " << pwm_1_in_use << endl;
			break;
		}
		case 19:
			break;
		case 40:
			break;
		case 41:
			break;
		case 45:
			break;
		case 52:
			break;
		case 53:
			break;
		default:
			cout << "Unsupported PWM pin: " << pin << endl;
			break;
	}

	//Clear the PWM BERR
	pwm_base->STA.BERR = 1;
	while (pwm_base->STA.BERR)
		;
	//PWMCtrl::DumpRegisters();
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

	if (-1 == pwm_1_in_use && -1 == pwm_2_in_use)
	{
		munmap((void *) gpio_base, sizeof(gpio_reg_t));
		gpio_base = NULL;

		munmap((void *) pwm_base, sizeof(pwm_ctrl_t));
		pwm_base = NULL;

		munmap((void *) clk_base, getpagesize());
		clk_base = NULL;
		CM_PERIICTL = NULL;
		CM_PERIIDIV = NULL;

		close(mem_fd);
		mem_fd = -1;
	}

}

/* Unfortunately, the description to clock manager of BCM2835/2827 is missing in the datasheet
 * I'm trying to "reverse-engineering" the wiringPi library along with information I can find
 * through Google.
 */
void PWMCtrl::SetClock(int32_t clk_div)
{
	uint32_t pwm_CTL = pwm_base->CTL.word;
	clk_div &= 4095;
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
}

void PWMCtrl::SetMode(uint32_t mode)
{
	pwm_reg_CTL_t temp;
	if (1 == m_pwm_channel)
	{
		temp.word = pwm_base->CTL.word;
		temp.MSEN1 = mode;
//		temp.PWEN1 = 1;
		pwm_base->CTL.word = temp.word;
	}
	else if (2 == m_pwm_channel)
	{
		temp.word = pwm_base->CTL.word;
		temp.MSEN2 = mode;
//		temp.PWEN2 = 1;
		pwm_base->CTL.word = temp.word;
	}
}

void PWMCtrl::SetRange(uint32_t range)
{
	if (1 == m_pwm_channel)
		pwm_base->RNG1 = range;
	else if (2 == m_pwm_channel)
		pwm_base->RNG2 = range;
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

void PWMCtrl::SetPWMCTL(const pwm_reg_CTL_t& ctl)
{
	pwm_base->CTL.word = ctl.word;
	usleep(200);
}

const volatile pwm_reg_CTL_t& PWMCtrl::GetPWMCTL()
{
	return pwm_base->CTL;
}

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

void PWMTest()
{
	//Note: In serializer mode, set the range to 32 since every word is only 32bits, or there will be gaps between FIFO words
	PWMCtrl pwm(18, 32, 8);
	pwm_reg_CTL_t pwm_ctl;
	pwm_ctl.word = pwm.GetPWMCTL().word;
	pwm_ctl.word &= 0xFFFFFF00;
	//Enable the PWM channel 1
	pwm_ctl.PWEN1 = 0;
	//Set to serialiser mode
	pwm_ctl.MODE1 = 1;
	//Use the FIFO
	pwm_ctl.USEF1 = 1;
	//Clear the FIFO
	pwm_ctl.CLRF1 = 1;

	pwm.SetPWMCTL(pwm_ctl);
	usleep(200);

	pwm_ctl.CLRF1 = 0;

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
		pwm_ctl.word = pwm.GetPWMCTL().word;
		pwm_ctl.PWEN1 = 1;
		pwm.SetPWMCTL(pwm_ctl);
		usleep(1000);

		pwm_ctl.word = pwm.GetPWMCTL().word;
		pwm_ctl.PWEN1 = 0;
		pwm_ctl.CLRF1 = 1;
		pwm.SetPWMCTL(pwm_ctl);
		sleep(1);
	}
}

