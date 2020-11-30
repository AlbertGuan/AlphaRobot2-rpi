/*
 * PCA9685Ctrl.h
 *
 *  Created on: May 9, 2019
 *      Author: Albert Guan
 *
 * Note: All table/page/chapter number in this module are PCA9685_datasheet.pdf
 * 	unless specified explicitly.
 *
 */

#pragma once

#include "GpioI2C.h"

/*
 * Don't get confused with the name of PCA9685: I2C bus controlled 16-channel LED
 * controller. It's a module which has 16 individual PWM output ports which can
 * be controlled through I2C bus.
 */

class PCA9685Ctrl
{
public:

	//
	// Definition of MODE1 registers, check Table 5 for more details
	//

	typedef union _MODE1Reg_ {
		struct {
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
	} MODE1Reg, *PMODE1Reg;

	//
	// TODO: We don't support programming MODE2 reg for now.
	//

	PCA9685Ctrl (
		_In_ int32_t PinSda,
		_In_ int32_t PinScl,
		_In_ const int8_t I2CAddr
		);

	virtual
	~PCA9685Ctrl (
		void
		);

	//
	// static routines to get register addresses.
	//

	static int8_t GetLEDxOnLowAddr(_In_ int32_t LEDIdx);
	static int8_t GetLEDxOnHighAddr(_In_ int32_t LEDIdx);
	static int8_t GetLEDxOffLowAddr(_In_ int32_t LEDIdx);
	static int8_t GetLEDxOffHighAddr(_In_ int32_t LEDIdx);

	int8_t GetMODE1Val();
	int32_t SetMODE1Val(_In_ uint8_t Val);
	int32_t UpdateFreq(_In_ float Freq);
	int32_t Sleep();
	int32_t Wakeup();
	int32_t Restart();

	int32_t
	SetPWMDutyCycle (
		_In_ int32_t ChannelIdx,
		_In_ float DutyCycle,
		_In_ int32_t RisingEdgeDelay = 0
		);

	int32_t
	SetPWMDutyCycle (
		_In_ float DutyCycle,
		_In_ int32_t RisingEdgeDelay = 0
		);

	//
	// The PCA9685 also supports programmable group I2C address, check
	//

	typedef enum _PCA9685GroupRegs_	{
		SUBADR1 = 0,
		SUBADR2,
		SUBADR3,
		ALLCALLADR
	} PCA9685GroupRegs, *PPCA9685GroupRegs;

private:

	//
	// The PCA9685 integrated with an oscillator can be used as the clock source
	// of the PWM output. The clock source can be switched through MODE1.EXTCLK.
	// The internal oscillator frequency is 25MHz.
	//

	const static double PCA9685_OSC_FREQ;

	//
	// Address of PCA9685 registers, check Table 4 for more details.
	//

	const static int8_t REG_MODE1_ADDR 				= 		0x00;
	const static int8_t REG_MODE2_ADDR 				= 		0x01;
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

	const int8_t m_I2CSlaveAddr;
	GpioI2C *m_I2CCtrl;
};
