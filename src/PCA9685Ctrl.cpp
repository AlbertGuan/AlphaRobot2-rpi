/*
 * PCA9685.cpp
 *
 *  Created on: May 9, 2019
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
#include <exception>
#include "PCA9685Ctrl.h"

//The I2C address PCA9685 is decided by pin A0-A5 (Fig 4 of PCA9685 Datasheet)
//After checking the AlphaRobot Pi schematic, all of them are connected to ground
//So the address is 0b1000000 -> 0x40, the leading 1 is fixed
const double PCA9685Ctrl::PCA9685_OSC_FREQ = 25000000.0f;
const int8_t PCA9685Ctrl::REG_GROUP_ADDR[4] = {0x5, 0x2, 0x3, 0x4};
int8_t PCA9685Ctrl::GetLEDxOnLowAddr(int32_t x)
{
	return REG_LED0_ON_LOW_ADDR + (x << 2);
}

int8_t PCA9685Ctrl::GetLEDxOnHighAddr(int32_t x)
{
	return REG_LED0_ON_HIGH_ADDR + (x << 2);
}

int8_t PCA9685Ctrl::GetLEDxOffLowAddr(int32_t x)
{
	return REG_LED0_OFF_LOW_ADDR + (x << 2);
}

int8_t PCA9685Ctrl::GetLEDxOffHighAddr(int32_t x)
{
	return REG_LED0_OFF_HIGH_ADDR + (x << 2);
}

PCA9685Ctrl::PCA9685Ctrl(int32_t sda, int32_t scl, const int8_t addr)
	: m_i2c_addr(addr)
{
	m_i2c = new GpioI2C(sda, scl);
}

PCA9685Ctrl::~PCA9685Ctrl()
{
	UpdateAllOutput(0.0);
	free(m_i2c);
	m_i2c = NULL;
}

int8_t PCA9685Ctrl::GetMODE1Val()
{
	int8_t re = 0;
	assert(m_i2c != NULL);
	m_i2c->read(m_i2c_addr, REG_MODE1_ADDR, &re);
	return re;
}

int32_t PCA9685Ctrl::SetMODE1Val(uint8_t val)
{
	assert(m_i2c != NULL);
	return m_i2c->write(m_i2c_addr, {REG_MODE1_ADDR, static_cast<int8_t>(val)});
}

int32_t PCA9685Ctrl::Sleep()
{
	MODE1Reg_t val;
	val.word = GetMODE1Val();
	val.SLEEP = 1;
	return SetMODE1Val(val.word);
}

int32_t PCA9685Ctrl::Wakeup()
{
	MODE1Reg_t val;
	val.word = GetMODE1Val();
	val.SLEEP = 0;
	return SetMODE1Val(val.word);
}

int32_t PCA9685Ctrl::Restart()
{
	MODE1Reg_t val;
	val.word = GetMODE1Val();
	val.RESTART = 1;
	return SetMODE1Val(val.word);
}

int32_t PCA9685Ctrl::UpdateFreq(float freq)
{
	assert(freq <= 1000.0 && freq >= 40.0);
	//Calculate the prescal value, refer to PRE_SCALE (page 25 of PCA9685 datasheet) for more details
	int8_t prescal = static_cast<int32_t>(PCA9685_OSC_FREQ / (4096 * freq) + 0.5);

	//According to the foot note at page 13, "Writes to PRE_SCALE register are blocked when SLEEP bit is logic 0 (MODE1)"
	//1. Set the sleep bit
	//2. Set the PRE_SCALE
	//3. Clear the sleep bit
	Sleep();
	std::cout << "Sleep" << std::endl;
	m_i2c->write(m_i2c_addr, {REG_PRE_SCALE_ADDR, prescal});
	std::cout << "Write" << std::endl;
	Wakeup();
	std::cout << "Wake Up" << std::endl;

	//According to foot node 2 at page 14, "It takes 500us max for the oscillator to be up and running once SLEEP bit has been
	//set to logic 0"
	usleep(1000);

	//Restart all PWM channels, chapter 7.3.1.1 for more details
	Restart();

	return 0;
}

int32_t PCA9685Ctrl::UpdatePWMOutput(int32_t idx, float duty_cycle, int32_t rising_edge_delay)
{
	assert(rising_edge_delay >= 0 && rising_edge_delay <= 4096);
	assert(duty_cycle >= 0.0 && duty_cycle <= 100.0);
	assert(idx >= 0 && idx <= 15);

	uint16_t off = static_cast<uint16_t>(4096 * duty_cycle + rising_edge_delay) & 0xFFF;
	int8_t on_low = static_cast<int8_t>(rising_edge_delay & 0xFF);
	int8_t on_high = static_cast<int8_t>(rising_edge_delay >> 8);
	int8_t off_low = static_cast<int8_t>(off & 0xFF);
	int8_t off_high = static_cast<int8_t>(off >> 8);
	m_i2c->write(m_i2c_addr, {GetLEDxOnLowAddr(idx), on_low});
	m_i2c->write(m_i2c_addr, {GetLEDxOnHighAddr(idx), on_high});
	m_i2c->write(m_i2c_addr, {GetLEDxOffLowAddr(idx), off_low});
	m_i2c->write(m_i2c_addr, {GetLEDxOffHighAddr(idx), off_high});
	return 0;
}

int32_t PCA9685Ctrl::UpdateAllOutput(float duty_cycle, int32_t rising_edge_delay)
{
	assert(rising_edge_delay >= 0 && rising_edge_delay <= 4096);
	assert(duty_cycle >= 0.0 && duty_cycle <= 100.0);

	uint16_t off = static_cast<uint16_t>(4096 * duty_cycle + rising_edge_delay) & 0xFFF;
	int8_t on_low = static_cast<int8_t>(rising_edge_delay & 0xFF);
	int8_t on_high = static_cast<int8_t>(rising_edge_delay >> 8);
	int8_t off_low = static_cast<int8_t>(off & 0xFF);
	int8_t off_high = static_cast<int8_t>(off >> 8);
	//Note: PCA9685's write format is <reg> <val>, don't put multiple sets in one packet
	m_i2c->write(m_i2c_addr, {REG_LEDALL_ON_LOW_ADDR, on_low});
	m_i2c->write(m_i2c_addr, {REG_LEDALL_ON_HIGH_ADDR, on_high});
	m_i2c->write(m_i2c_addr, {REG_LEDALL_OFF_LOW_ADDR, off_low});
	m_i2c->write(m_i2c_addr, {REG_LEDALL_OFF_HIGH_ADDR, off_high});
	return 0;
}


