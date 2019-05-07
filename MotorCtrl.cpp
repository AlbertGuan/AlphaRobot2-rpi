/*
 * MotorCtrl.cpp
 *
 *  Created on: May 5, 2019
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
#include "MotorCtrl.h"

int32_t MotorCtrl::mem_fd = -1;
volatile gpio_reg_t *MotorCtrl::gpio_base = NULL;

MotorCtrl::MotorCtrl()
{
	AddrInit();

	SetPinSelection(GPIO_AIN1, FSEL_OUTPUT);
	SetPinSelection(GPIO_AIN2, FSEL_OUTPUT);
	SetPinSelection(GPIO_PWMA, FSEL_OUTPUT);
	SetPinSelection(GPIO_BIN1, FSEL_OUTPUT);
	SetPinSelection(GPIO_BIN2, FSEL_OUTPUT);
	SetPinSelection(GPIO_PWMB, FSEL_OUTPUT);

	gpio_base->out_clear[0] = (0x1 << GPIO_AIN1) | (0x1 << GPIO_AIN2) | (0x1 << GPIO_BIN1) | (0x1 << GPIO_BIN2);
	gpio_base->out_set[0] = (0x1 << GPIO_PWMA) | (0x1 << GPIO_PWMB);
}

void MotorCtrl::SetPinSelection(uint32_t pin, enum GPIO_FUN_SELECT alt)
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

void MotorCtrl::ShortBrake()
{
	//Short Brake:
	//IN1: High
	//IN2: High
	gpio_base->out_set[0] = (0x1 << GPIO_AIN1) | (0x1 << GPIO_AIN2) | (0x1 << GPIO_BIN1) | (0x1 << GPIO_BIN2);
	std::bitset<32> b(gpio_base->out_set[0]);
	std::cout << "out_set:   " << b << std::endl;
	printf("Short Brake: AIN1 High, AIN2 High, BIN1 High, BIN2 High\n");
}

void MotorCtrl::CCW()
{
	//CCW:
	//IN1: Low
	//IN2: High
	gpio_base->out_clear[0] = (0x1 << GPIO_AIN1) | (0x1 << GPIO_BIN1);
	gpio_base->out_set[0] = (0x1 << GPIO_AIN2) | (0x1 << GPIO_BIN2);
	std::bitset<32> b(gpio_base->out_set[0]);
	std::cout << "out_set:   " << b << std::endl;
	std::bitset<32> a(gpio_base->out_clear[0]);
	std::cout << "out_clear: " << a << std::endl;
	printf("Counter clock-wise: AIN1 Low, AIN2 high, BIN1 Low, BIN2 high\n");
}

void MotorCtrl::CW()
{
	//CW:
	//IN1: High
	//IN2: Low
	gpio_base->out_set[0] = (0x1 << GPIO_AIN1) | (0x1 << GPIO_BIN1);
	gpio_base->out_clear[0] = (0x1 << GPIO_AIN2) | (0x1 << GPIO_BIN2);
	printf("Clock-wise: AIN1 high, AIN2 Low, BIN1 high, BIN2 Low\n");
}

void MotorCtrl::Stop()
{
	//Stop:
	//IN1: Low
	//IN2: Low
	gpio_base->out_clear[0] = (0x1 << GPIO_AIN1) | (0x1 << GPIO_AIN2) | (0x1 << GPIO_BIN1) | (0x1 << GPIO_BIN2);
	printf("Stop Motor: AIN1 Low, AIN2 Low, BIN1 Low, BIN2 Low\n");
}

void MotorCtrl::AddrInit()
{
	if (mem_fd != 0)
	{
		if ((mem_fd = open("/dev/mem", O_RDWR | O_SYNC | O_CLOEXEC)) < 0)
		{
			//Error Handler
			std::cout << "MotorCtrl Constructor: Failed to open /dev/mem" << std::endl;
			return;
		}

		if (MAP_FAILED == (gpio_base = (gpio_reg_t *) mmap(NULL, sizeof(gpio_reg_t), PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, GPIO_BASE_PHY_ADDR)))
		{
			//Error Handler
			std::cout << "MotorCtrl Constructor: Failed to mmap gpio_base" << std::endl;
		}
	}
}

void MotorDemo()
{
	using namespace std;
	MotorCtrl motor;
	while (1)
	{
		motor.Stop();

		sleep(1);
		getchar();
		motor.CW();
		sleep(5);
		getchar();
		motor.ShortBrake();
		sleep(1);
		getchar();
		motor.CCW();
		sleep(5);
		getchar();

	}
}
