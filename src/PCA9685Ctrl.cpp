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

const double PCA9685Ctrl::PCA9685_OSC_FREQ = 25000000.0f;
const int8_t PCA9685Ctrl::REG_GROUP_ADDR[4] = {0x2, 0x3, 0x4, 0x5};

int8_t
PCA9685Ctrl::GetLEDxOnLowAddr (
	_In_ int32_t LEDIdx
	)

/*
 Routine Description:

	This routine returns the address of LEDx_ON_L.

 Parameters:

	LEDIdx - Supplies the index of the LED to query.

 Return Value:

	int8_t - Supplies the address of the register.

*/

{

	return REG_LED0_ON_LOW_ADDR + (LEDIdx << 2);
}

int8_t
PCA9685Ctrl::GetLEDxOnHighAddr (
	_In_ int32_t LEDIdx
	)

/*
 Routine Description:

	This routine returns the address of LEDx_ON_H.

 Parameters:

	LEDIdx - Supplies the index of the LED to query.

 Return Value:

	int8_t - Supplies the address of the register.

*/

{

	return REG_LED0_ON_HIGH_ADDR + (LEDIdx << 2);
}

int8_t
PCA9685Ctrl::GetLEDxOffLowAddr (
	_In_ int32_t LEDIdx
	)

/*
 Routine Description:

	This routine returns the address of LEDx_OFF_L.

 Parameters:

	LEDIdx - Supplies the index of the LED to query.

 Return Value:

	int8_t - Supplies the address of the register.

*/

{

	return REG_LED0_OFF_LOW_ADDR + (LEDIdx << 2);
}

int8_t
PCA9685Ctrl::GetLEDxOffHighAddr (
	_In_ int32_t LEDIdx
	)

/*
 Routine Description:

	This routine returns the address of LEDx_OFF_H.

 Parameters:

	LEDIdx - Supplies the index of the LED to query.

 Return Value:

	int8_t - Supplies the address of the register.

*/

{
	return REG_LED0_OFF_HIGH_ADDR + (LEDIdx << 2);
}

PCA9685Ctrl::PCA9685Ctrl (
	_In_ int32_t PinSda,
	_In_ int32_t PinScl,
	_In_ const int8_t I2CAddr
	) : m_I2CSlaveAddr(I2CAddr)

/*
 Routine Description:

	This is the constructor of PCA9685Ctrl, it inits an instance of I2C control.

 Parameters:

 	PinSda - Supplies the pin number of SDA.

 	PinScl - Supplies the pin number of SCL.

 	I2CAddress - Supplies the I2C slave address of this module.

 Return Value:

	None.

*/

{

	m_I2CCtrl = new GpioI2C(PinSda, PinScl);
	return;
}

PCA9685Ctrl::~PCA9685Ctrl (
	void
	)

/*
 Routine Description:

	This is the destructor of PCA9685Ctrl, it stops this module and free I2C control
	instance.

 Parameters:

 	None.

 Return Value:

	None.

*/

{

	SetPWMDutyCycle(0.0);
	if (m_I2CCtrl != NULL) {
		delete m_I2CCtrl;
		m_I2CCtrl = NULL;
	}

	return;
}

int8_t
PCA9685Ctrl::GetMODE1Val (
	void
	)

/*
 Routine Description:

	This routine returns the current value of register MODE1.

 Parameters:

	None.

 Return Value:

	int8_t - Supplies the value of register MODE1.

*/

{

	int8_t re = 0;
	assert(m_I2CCtrl != NULL);
	if (m_I2CCtrl != NULL) {
		m_I2CCtrl->read(m_I2CSlaveAddr, REG_MODE1_ADDR, &re);
	}

	return re;
}

int32_t
PCA9685Ctrl::SetMODE1Val (
	_In_ uint8_t val
	)

/*
 Routine Description:

	This routine sets register MODE1.

 Parameters:

	Val - Supplies the value to write to MODE1.

 Return Value:

	int8_t - Supplies the number of bytes written to MODE1.

*/

{

	int8_t re = 0;

	assert(m_I2CCtrl != NULL);
	if (m_I2CCtrl != NULL) {
		re = m_I2CCtrl->write(m_I2CSlaveAddr,
							  {REG_MODE1_ADDR, static_cast<int8_t>(val)});
	}

	return re;
}

int32_t
PCA9685Ctrl::Sleep (
	void
	)

/*
 Routine Description:

	This routine puts the module to sleep.

 Parameters:

	None.

 Return Value:

	int8_t - Supplies the number of bytes written to the module.

*/

{
	MODE1Reg val;
	val.word = GetMODE1Val();
	val.SLEEP = 1;
	return SetMODE1Val(val.word);
}

int32_t
PCA9685Ctrl::Wakeup (
	void
	)

/*
 Routine Description:

	This routine wakes up the module.

 Parameters:

	None.

 Return Value:

	int8_t - Supplies the number of bytes written to the module.

*/

{

	MODE1Reg val;
	val.word = GetMODE1Val();
	val.SLEEP = 0;
	return SetMODE1Val(val.word);
}

int32_t
PCA9685Ctrl::Restart (
	void
	)

/*
 Routine Description:

	This routine resets the module.

 Parameters:

	None.

 Return Value:

	int8_t - Supplies the number of bytes written to the module.

*/

{

	MODE1Reg val;
	val.word = GetMODE1Val();
	val.RESTART = 1;
	return SetMODE1Val(val.word);
}

int32_t
PCA9685Ctrl::UpdateFreq (
	_In_ float Freq
	)

/*
 Routine Description:

	This routine sets the output PWM frequency.

 Parameters:

	Freq - Supplies the frequency of the output.

 Return Value:

	int8_t - Supplies the number of bytes written to the module.

*/

{

	assert(Freq <= 1000.0 && Freq >= 40.0);

	RPI_PRINT_EX(InfoLevelDebug,
				 "Update the PCA9685 output to %f Hz",
				 Freq);

	//
	//Calculate the prescal value, refer to PRE_SCALE (page 25 of PCA9685 datasheet) for more details
	//

	int8_t prescal = static_cast<int32_t>(PCA9685_OSC_FREQ / (4096 * Freq) + 0.5);

	//
	// According to the foot note at page 13, "Writes to PRE_SCALE register are blocked when SLEEP bit is logic 0 (MODE1)"
	// 1. Set the sleep bit
	// 2. Set the PRE_SCALE
	// 3. Clear the sleep bit
	//

	Sleep();
	m_I2CCtrl->write(m_I2CSlaveAddr, {REG_PRE_SCALE_ADDR, prescal});
	Wakeup();

	//
	// According to foot node 2 at page 14, "It takes 500us max for the oscillator
	// to be up and running once SLEEP bit has been set to logic 0"
	//

	usleep(1000);

	//
	//Restart all PWM channels, chapter 7.3.1.1 for more details
	//

	Restart();
	return 0;
}

int32_t
PCA9685Ctrl::SetPWMDutyCycle (
	_In_ int32_t ChannelIdx,
	_In_ float DutyCycle,
	_In_ int32_t RisingEdgeDelay
	)

/*
 Routine Description:

	This routine sets the output PWM duty cycle to specific channel.

 Parameters:

	ChannelIdx - Supplies the channel to set the duty cycle.

	DutyCycle - Supplies the duty cycle of the PWM output.

	RisingEdgeDelay - Supplies the delay of the rising edge.

 Return Value:

	int8_t - Supplies the number of bytes written to the module.

*/

{

	assert(RisingEdgeDelay >= 0 && RisingEdgeDelay <= 4096);
	assert(DutyCycle >= 0.0 && DutyCycle <= 100.0);
	assert(ChannelIdx >= 0 && ChannelIdx <= 15);

	uint16_t off = static_cast<uint16_t>(4096 * DutyCycle + RisingEdgeDelay) & 0xFFF;
	int8_t on_low = static_cast<int8_t>(RisingEdgeDelay & 0xFF);
	int8_t on_high = static_cast<int8_t>(RisingEdgeDelay >> 8);
	int8_t off_low = static_cast<int8_t>(off & 0xFF);
	int8_t off_high = static_cast<int8_t>(off >> 8);

	//
	// Note: PCA9685's write format is <reg> <val>, don't put multiple sets in one packet
	//

	m_I2CCtrl->write(m_I2CSlaveAddr, {GetLEDxOnLowAddr(ChannelIdx), on_low});
	m_I2CCtrl->write(m_I2CSlaveAddr, {GetLEDxOnHighAddr(ChannelIdx), on_high});
	m_I2CCtrl->write(m_I2CSlaveAddr, {GetLEDxOffLowAddr(ChannelIdx), off_low});
	m_I2CCtrl->write(m_I2CSlaveAddr, {GetLEDxOffHighAddr(ChannelIdx), off_high});
	return 0;
}

int32_t
PCA9685Ctrl::SetPWMDutyCycle (
	_In_ float DutyCycle,
	_In_ int32_t RisingEdgeDelay
	)

/*
 Routine Description:

	This routine sets the output PWM duty cycle to all channels.

 Parameters:

	DutyCycle - Supplies the duty cycle of the PWM output.

	RisingEdgeDelay - Supplies the delay of the rising edge.

 Return Value:

	int8_t - Supplies the number of bytes written to the module.

*/

{

	assert(RisingEdgeDelay >= 0 && RisingEdgeDelay <= 4096);
	assert(DutyCycle >= 0.0 && DutyCycle <= 100.0);

	uint16_t off = static_cast<uint16_t>(4096 * DutyCycle + RisingEdgeDelay) & 0xFFF;
	int8_t on_low = static_cast<int8_t>(RisingEdgeDelay & 0xFF);
	int8_t on_high = static_cast<int8_t>(RisingEdgeDelay >> 8);
	int8_t off_low = static_cast<int8_t>(off & 0xFF);
	int8_t off_high = static_cast<int8_t>(off >> 8);

	//
	// Note: PCA9685's write format is <reg> <val>, don't put multiple sets in one packet
	//

	m_I2CCtrl->write(m_I2CSlaveAddr, {REG_LEDALL_ON_LOW_ADDR, on_low});
	m_I2CCtrl->write(m_I2CSlaveAddr, {REG_LEDALL_ON_HIGH_ADDR, on_high});
	m_I2CCtrl->write(m_I2CSlaveAddr, {REG_LEDALL_OFF_LOW_ADDR, off_low});
	m_I2CCtrl->write(m_I2CSlaveAddr, {REG_LEDALL_OFF_HIGH_ADDR, off_high});
	return 0;
}
