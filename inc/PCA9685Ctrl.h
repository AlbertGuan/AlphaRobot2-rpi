/*
 * PCA9685Ctrl.h
 *
 *  Created on: May 9, 2019
 *      Author: aguan
 */

#pragma once

#include "GpioI2C.h"

class PCA9685Ctrl
{
public:
	typedef union
	{
		struct
		{
			uint8_t ALLCALL		: 1;	//1: Responds to LED All Call I2C address
			uint8_t SUB3		: 1;	//1: Responds to SUB3 Call I2C address
			uint8_t SUB2		: 1;	//1: Responds to SUB2 Call I2C address
			uint8_t SUB1		: 1;	//1: Responds to SUB1 Call I2C address
			uint8_t SLEEP		: 1;	//1: Low power mode, oscillator off
			uint8_t AI			: 1;	//1: Register auto-increment
			uint8_t EXTCLK		: 1;	//1: Use EXTCLK pin clock
			uint8_t RESTART		: 1;	//1: Restart enable
		};
		uint8_t word;
	}MODE1Reg_t;

	PCA9685Ctrl(int32_t sda, int32_t scl, const int8_t addr);
	virtual ~PCA9685Ctrl();
	static int8_t GetLEDxOnLowAddr(int32_t x);
	static int8_t GetLEDxOnHighAddr(int32_t x);
	static int8_t GetLEDxOffLowAddr(int32_t x);
	static int8_t GetLEDxOffHighAddr(int32_t x);

	int8_t GetMODE1Val();
	int32_t SetMODE1Val(uint8_t val);
	int32_t UpdateFreq(float freq);
	int32_t Sleep();
	int32_t Wakeup();
	int32_t Restart();
	int32_t UpdatePWMOutput(int32_t idx, float duty_cycle, int32_t rising_edge_delay = 0);
	int32_t UpdateAllOutput(float duty_cycle, int32_t rising_edge_delay = 0);

	enum reg_group_addresses
	{
		ALLCALLADR		= 0,
		SUBADR1			= 1,
		SUBADR2			= 2,
		SUBADR3			= 3
	};
private:

	const static double PCA9685_OSC_FREQ;

	const static int8_t REG_MODE1_ADDR 				= 		0x00;
	const static int8_t REG_MODE2_ADDR 				= 		0x01;
	//Table 4. Register Summary of PCA9685 datasheet
	//REG_GROUP_ADDR[0] = 0x5: ALLCALLADR
	//REG_GROUP_ADDR[1] = 0x2: SUBADR1
	//REG_GROUP_ADDR[2] = 0x3: SUBADR2
	//REG_GROUP_ADDR[3] = 0x4: SUBADR3
	const static int8_t REG_GROUP_ADDR[4];
	const static int8_t REG_LED0_ON_LOW_ADDR		=		0x06;
	const static int8_t REG_LED0_ON_HIGH_ADDR		=		0x07;
	const static int8_t REG_LED0_OFF_LOW_ADDR		=		0x08;
	const static int8_t REG_LED0_OFF_HIGH_ADDR		=		0x09;
	const static int8_t REG_LEDALL_ON_LOW_ADDR		=		0xFA;
	const static int8_t REG_LEDALL_ON_HIGH_ADDR		=		0xFB;
	const static int8_t REG_LEDALL_OFF_LOW_ADDR		=		0xFC;
	const static int8_t REG_LEDALL_OFF_HIGH_ADDR	=		0xFD;
	const static int8_t REG_PRE_SCALE_ADDR			=		0xFE;

	const int8_t m_i2c_addr;
	GpioI2C *m_i2c;
};
