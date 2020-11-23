/*
 * MotorCtrl.cpp
 *
 *  Created on: May 5, 2019
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
#include "MotorCtrl.h"


MotorCtrl::MotorCtrl()
{
	m_pins.push_back(new GpioOut(GPIO_AIN1));
	m_pins.push_back(new GpioOut(GPIO_AIN2));
	m_pins.push_back(new GpioOut(GPIO_PWMA));
	m_pins.push_back(new GpioOut(GPIO_BIN1));
	m_pins.push_back(new GpioOut(GPIO_BIN2));
	m_pins.push_back(new GpioOut(GPIO_PWMB));

	GpioOut::Update(std::vector<uint32_t>{GPIO_PWMA, GPIO_PWMB}, std::vector<uint32_t>{GPIO_AIN1, GPIO_AIN2, GPIO_BIN1, GPIO_BIN2});
}

MotorCtrl::~MotorCtrl()
{
	Stop();
	for (auto &pin : m_pins)
		free(pin);
}

void MotorCtrl::ShortBrake()
{
	//Short Brake:
	//IN1: High
	//IN2: High
	GpioOut::Update(std::vector<uint32_t> {GPIO_AIN1, GPIO_AIN2, GPIO_BIN1, GPIO_BIN2}, std::vector<uint32_t> {});
}

void MotorCtrl::CCW()
{
	//CCW:
	//IN1: Low
	//IN2: High
	GpioOut::Update(std::vector<uint32_t> {GPIO_AIN2, GPIO_BIN2}, std::vector<uint32_t> {GPIO_AIN1, GPIO_BIN1});
	printf("Counter clock-wise: AIN1 Low, AIN2 high, BIN1 Low, BIN2 high\n");
}

void MotorCtrl::CW()
{
	//CW:
	//IN1: High
	//IN2: Low
	GpioOut::Update(std::vector<uint32_t> {GPIO_AIN1, GPIO_BIN1}, std::vector<uint32_t> {GPIO_AIN2, GPIO_BIN2});
	printf("Clock-wise: AIN1 high, AIN2 Low, BIN1 high, BIN2 Low\n");
}

void MotorCtrl::Stop()
{
	//Stop:
	//IN1: Low
	//IN2: Low
	GpioOut::Update(std::vector<uint32_t> {}, std::vector<uint32_t> {GPIO_AIN1, GPIO_AIN2, GPIO_BIN1, GPIO_BIN2});
	printf("Stop Motor: AIN1 Low, AIN2 Low, BIN1 Low, BIN2 Low\n");
}


void MotorDemo()
{
	using namespace std;
	MotorCtrl motor;
	while (1)
	{
		motor.Stop();
		sleep(1);
		motor.CW();
		sleep(5);
		motor.ShortBrake();
		sleep(1);
		motor.CCW();
		sleep(5);
	}
}
