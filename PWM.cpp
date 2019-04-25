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
		if ((mem_fd = open ("/dev/mem", O_RDWR | O_SYNC | O_CLOEXEC)) < 0)
		{
			//Error Handler
			std::cout << "PWMCtrl Constructor: Failed to open /dev/mem" << std::endl;
			return;
		}

		if (MAP_FAILED == (gpio_base = (gpio_reg_t *)mmap(NULL, BLOCK_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, mem_fd, GPIO_BASE_PHY_ADDR)))
		{
			//Error Handler
			std::cout << "PWMCtrl Constructor: Failed to mmap gpio_base" << std::endl;
		}

		if (MAP_FAILED == (pwm_base = (pwm_ctrl_t *)mmap(NULL, BLOCK_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, mem_fd, GPIO_PWM_PHY_ADDR)))
		{
			//Error Handler
			std::cout << "PWMCtrl Constructor: Failed to mmap pwm_base" << std::endl;
		}

		if (MAP_FAILED == (clk_base = (uint32_t *)mmap(NULL, BLOCK_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, mem_fd, GPIO_CLK_PHY_ADDR)))
		{
			//Error Handler
			std::cout << "PWMCtrl Constructor: Failed to mmap clk_base" << std::endl;
		}
		CM_PERIICTL = (uint32_t *)((uint32_t)clk_base + CM_PERIICTL_OFFSET);
		CM_PERIIDIV = (uint32_t *)((uint32_t)clk_base + CM_PERIIDIV_OFFSET);
	}

	//PrintAddress();

//	DumpRegisters();
}

void PWMCtrl::PrintAddress()
{
	using namespace std;
	cout << "pwm_base->CTL:\t\t0x" << setw(8) << hex << (uint32_t)&pwm_base->CTL << endl;
	cout << "pwm_base->STA:\t\t0x" << setw(8) << hex << (uint32_t)&pwm_base->STA << endl;
	cout << "pwm_base->DMAC:\t\t0x" << setw(8) << hex << (uint32_t)&pwm_base->DMAC << endl;
	cout << "pwm_base->RNG1:\t\t0x" << setw(8) << hex << (uint32_t)&pwm_base->RNG1 << endl;
	cout << "pwm_base->DAT1:\t\t0x" << setw(8) << hex << (uint32_t)&pwm_base->DAT1 << endl;
	cout << "pwm_base->FIF1:\t\t0x" << setw(8) << hex << (uint32_t)&pwm_base->FIF1 << endl;
	cout << "pwm_base->RNG2:\t\t0x" << setw(8) << hex << (uint32_t)&pwm_base->RNG2 << endl;
	cout << "pwm_base->DAT2:\t\t0x" << setw(8) << hex << (uint32_t)&pwm_base->DAT2 << endl;
}

void PWMCtrl::DumpRegisters()
{
	using namespace std;
	for (int i = 0; i < 6; ++i)
		cout << "gpio_base->select[" << i << "]:\t\tAddr: 0x" << setw(8) << hex << (uint32_t)&gpio_base->select[i] << "\t Val: " << setw(8) << hex << gpio_base->select[i] << endl;
	cout << endl;

	cout << "pwm_base->CTL:\t\tAddr: 0x" << setw(8) << hex << (uint32_t)&pwm_base->CTL << "\t Val: " << setw(8) << hex << pwm_base->CTL.word << endl;
	cout << "pwm_base->STA:\t\tAddr: 0x" << setw(8) << hex << (uint32_t)&pwm_base->STA << "\t Val: " << setw(8) << hex << pwm_base->STA.word << endl;
	cout << "pwm_base->DMAC:\t\tAddr: 0x" << setw(8) << hex << (uint32_t)&pwm_base->DMAC << "\t Val: " << setw(8) << hex << pwm_base->DMAC.word << endl;
	cout << "pwm_base->RNG1:\t\tAddr: 0x" << setw(8) << hex << (uint32_t)&pwm_base->RNG1 << "\t Val: " << setw(8) << hex << pwm_base->RNG1 << endl;
	cout << "pwm_base->DAT1:\t\tAddr: 0x" << setw(8) << hex << (uint32_t)&pwm_base->DAT1 << "\t Val: " << setw(8) << hex << pwm_base->DAT1 << endl;
	cout << "pwm_base->FIF1:\t\tAddr: 0x" << setw(8) << hex << (uint32_t)&pwm_base->FIF1 << "\t Val: " << setw(8) << hex << pwm_base->FIF1 << endl;
	cout << "pwm_base->RNG2:\t\tAddr: 0x" << setw(8) << hex << (uint32_t)&pwm_base->RNG2 << "\t Val: " << setw(8) << hex << pwm_base->RNG2 << endl;
	cout << "pwm_base->DAT2:\t\tAddr: 0x" << setw(8) << hex << (uint32_t)&pwm_base->DAT2 << "\t Val: " << setw(8) << hex << pwm_base->DAT2 << endl;
	cout << endl;

	cout << "CM_PERIICTL:\t\tAddr: 0x" << setw(8) << hex << (uint32_t)CM_PERIICTL << "\t Val: " << setw(8) << hex << *CM_PERIICTL << endl;
	cout << "CM_PERIIDIV:\t\tAddr: 0x" << setw(8) << hex << (uint32_t)CM_PERIIDIV << "\t Val: " << setw(8) << hex << *CM_PERIIDIV << endl;
}

PWMCtrl::PWMCtrl()
{
	PWMCtrl::PWMAddrInit();
}

PWMCtrl::PWMCtrl(int32_t pin, int32_t mode, int32_t range, int32_t divisor)
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
				cout << "Setting pin 18 to PWM" << endl;
				cout << "pin selection[1] before changing: " << hex << gpio_base->select[1] << endl;
				//Note: Value of FSEL_ALT are not 0-5
				set_bits((uint32_t *)&gpio_base->select[1], FSEL_ALT_5, 26, 24);
				cout << "pin selection[1] after changing: " << hex << gpio_base->select[1] << endl;
				usleep(200);
				cout << "Set mode to " << dec <<mode << endl;
				SetMode(mode);
				cout << "Set range to " << dec << range << endl;
				SetRange(range);
				usleep(10);
				cout << "Set clock divisor to " << dec << divisor << endl;
				SetClock(divisor);
				cout << "PWM frequency is: " << PWMCtrl::PWM_CLK_SRC_REQ  / range / divisor << endl;
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
		//ToDo: unmap and close
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
		usleep (1);

	*CM_PERIIDIV = BCM_PASSWORD | (clk_div << 12) ;

	*CM_PERIICTL = BCM_PASSWORD | 0x11 ;	// Start PWM clock
	pwm_base->CTL.word = pwm_CTL ;			// restore PWM_CONTROL
}

void PWMCtrl::SetMode(uint32_t mode)
{
	if (1 == m_pwm_channel)
	{
		pwm_base->CTL.word &= 0xFFFFFF00;
		pwm_base->CTL.MSEN1 = mode;
		pwm_base->CTL.PWEN1 = 1;
	}
	else if (2 == m_pwm_channel)
	{
		pwm_base->CTL.word &= 0xFFFF00FF;
		pwm_base->CTL.MSEN2 = mode;
		pwm_base->CTL.PWEN2 = 1;
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


void PWMTest()
{
	PWMCtrl pwm(18, PWMCtrl::PWM_MODE_M_S, 100, 24);
	while (1)
	{
		pwm.pwmWrite(64);
		usleep(1000);
	}
}

