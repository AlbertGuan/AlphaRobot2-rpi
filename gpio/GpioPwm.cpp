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

volatile GpioPwm::PWMCtrlRegisters *GpioPwm::PWMCtrlRegs = NULL;
volatile uint32_t *GpioPwm::ClkRegisters = NULL;
volatile uint32_t *GpioPwm::CM_PWMCTL = NULL;
volatile uint32_t *GpioPwm::CM_PWMDIV = NULL;

int32_t GpioPwm::NumOfPWMInstances = 0;
int32_t GpioPwm::PWM1InUse = CHANNEL_NOT_IN_USE;
int32_t GpioPwm::PWM2InUse = CHANNEL_NOT_IN_USE;

GPIO_FUN_SELECT
GpioPwm::GetPinSelection (
	_In_ uint32_t Pin
)

/*
 Routine Description:

	This routine gets pin selections. It is based on
	CH9.5 of BCM2837 ARM Peripheral.pdf

	TODO: This routine is based on BCM2837 which might not be platform compatible.

 Parameters:

 	Pin - Supplies the pin number.

 Return Value:

	int32_t - Supplies the pin selection.

*/

{

	GPIO_FUN_SELECT re;

	re = FSEL_INPUT;
	switch(Pin) {
	case 12:
	case 13:
	case 40:
	case 41:
	case 45:
		re = FSEL_ALT_0;
		break;

	case 52:
	case 53:
		re = FSEL_ALT_1;
		break;

	case 18:
	case 19:
		re = FSEL_ALT_5;
		break;

	default:
		RPI_PRINT_EX(InfoLevelError,
					 "Pin %d doesn't support PWM!",
					 Pin);

		assert(false);
		break;
	}

	return re;
}

int32_t
GpioPwm::GetChannel (
	void
	)

/*
 Routine Description:

	This routine gets PWM channel ID from pin numbers. It is based on
	CH9.5 of BCM2837 ARM Peripheral.pdf

	TODO: This routine is based on BCM2837 which might not be platform compatible.

 Parameters:

 	None.

 Return Value:

	Error code.

*/

{

	int32_t Error;

	Error = ERROR_SUCCESS;
	try	{
		switch (m_Pins[0]) {
		case 12:
		case 18:
		case 40:
		case 52:
			if (PWM1InUse == CHANNEL_NOT_IN_USE) {
				m_PWMChannelId = 1;
				PWM1InUse = m_Pins[0];

			} else {
				Error = ERROR_CHANNEL_OCCUPIED;
				throw "Failed to init pin " + std::to_string(m_Pins[0]) +
					  " occupied by " + std::to_string(PWM1InUse);
			}

			break;

		case 13:
		case 19:
		case 41:
		case 45:
		case 53:
			if (PWM2InUse == CHANNEL_NOT_IN_USE) {
				m_PWMChannelId = 2;
				PWM2InUse = m_Pins[0];

			} else {
				Error = ERROR_CHANNEL_OCCUPIED;
				throw "Failed to init pin " + std::to_string(m_Pins[0]) +
					  " occupied by " + std::to_string(PWM2InUse);
			}

			break;

		default:
			Error = ERROR_INVALID_PIN;
			throw "Pin " + std::to_string(m_Pins[0]) + " doesn't support PWM";
			break;
		}

	} catch (const std::string &exp) {
		RPI_PRINT_EX(InfoLevelError, "exception %s", exp.c_str());

	} catch (...) {
		Error = ERROR_UNKNOWN;
		RPI_PRINT(InfoLevelError, "unknown exception");
	}

	return Error;
}

int32_t
GpioPwm::Init (
	void
	)

/*
 Routine Description:

	This routine maps PWM registers.

 Parameters:

 	None.

 Return Value:

	int32_t - TODO: Have an error code map when we have more errors.

*/

{
	int32_t Error;

	Error = ERROR_SUCCESS;
	if (0 != NumOfPWMInstances) {
		goto InitEnd;
	}

	try {
		PWMCtrlRegs = const_cast<volatile PWMCtrlRegisters *>(static_cast<PWMCtrlRegisters *>(
								mmap(NULL,
								 	 sizeof(PWMCtrlRegisters),
									 PROT_READ | PROT_WRITE,
									 MAP_SHARED,
									 mem_fd,
									 GPIO_PWM_PHY_ADDR)));

		if (MAP_FAILED == PWMCtrlRegs) {
			Error = ERROR_FAILED_MEM_MAP;
			throw "Failed to mmap PWMCtrlRegs";
		}

		ClkRegisters = const_cast<volatile uint32_t *>(static_cast<uint32_t *>(
								mmap(NULL,
									 BLOCK_SIZE,
									 PROT_READ | PROT_WRITE,
									 MAP_SHARED,
									 mem_fd,
									 GPIO_CLK_PHY_ADDR)));

		if (MAP_FAILED == ClkRegisters) {
			Error = ERROR_FAILED_MEM_MAP;
			throw "Failed to mmap ClkRegisters";
		}

	} catch (const std::string &exp) {
		RPI_PRINT_EX(InfoLevelError, "%s", exp.c_str());

	} catch (...) {
		Error = ERROR_UNKNOWN;
		RPI_PRINT(InfoLevelError, "got an unknown exception, try to run with root");
	}

	CM_PWMCTL = ADDRESS_TO_VOLATILE_POINTER((uint32_t) ClkRegisters + CM_PWMCTL_OFFSET);
	CM_PWMDIV = ADDRESS_TO_VOLATILE_POINTER((uint32_t) ClkRegisters + CM_PWMDIV_OFFSET);

InitEnd:
	return Error;
}

int32_t
GpioPwm::Uninit (
	void
	)

/*
 Routine Description:

	This routine unmaps PWM registers.

 Parameters:

 	None.

 Return Value:

	int32_t

*/

{
	if (0 == NumOfPWMInstances) {
		if (PWMCtrlRegs != NULL) {
			munmap(static_cast<void *>(const_cast<PWMCtrlRegisters *>(PWMCtrlRegs)),
				   sizeof(PWMCtrlRegisters));

			PWMCtrlRegs = NULL;
		}

		if (ClkRegisters != NULL) {
			munmap(static_cast<void *>(const_cast<uint32_t *>(ClkRegisters)), BLOCK_SIZE);
			ClkRegisters = NULL;
			CM_PWMCTL = NULL;
			CM_PWMDIV = NULL;
		}
	}

	return ERROR_SUCCESS;
}

GpioPwm::GpioPwm (
	_In_ int32_t Pin,
	_In_ int32_t Range,
	_In_ int32_t Divisor,
	_In_ int32_t Mode,
	_In_ int32_t Fifo
	) : GpioBase({Pin}, GetPinSelection(Pin))

/*
 Routine Description:

	This routine is the constructor of GpioPwm, it initializes PWM related
	registers.

 Parameters:

 	Pin - Supplies the pin number of the PWM output.

 	Range - Supplies the range of this channel.

 	Divisor - Supplies the divisor of the clock. PWM Freq = 19.2MHz / Range / Divisor.

 	Mode - Supplies serial or normal mode.

 	Fifo - Supplies using FIFO or not.

 Return Value:

	None.

*/

{
	//
	//Step 0: Map PWM and CLK registers
	//

	GpioPwm::Init();

	//
	//Step 1: Figure out which PWM channel to use
	//

	GetChannel();

	//
	//Step 2: Turn off the PWM before making changes
	//

	PWMOnOff(OFF);

	//
	//Step 3: Setup CTL register
	//

	SetPWMCtrl(Mode, Fifo);

	//
	//Step 4: Setup PWM range
	//

	SetRange(Range);

	//
	//Step 5: Setup PWM Clock
	//

	SetClock(Divisor);
	RPI_PRINT_EX(InfoLevelDebug,
				 "PWM Frequency is set to %f",
				 (float)PWM_CLK_SRC_REQ / Range / Divisor);

	//
	//Step 6: Acknowledge the PWM BERR
	//

	PWMCtrlRegs->STA.BERR = 1;
	while (PWMCtrlRegs->STA.BERR);
	NumOfPWMInstances += 1;

	//
	//Note: The PWM hasn't been enabled yet!!!
	//

	return;
}

GpioPwm::~GpioPwm (
	void
	)

/*
 Routine Description:

	This routine is the destructor of GpioPwm.
	registers.

 Parameters:

 	None.

 Return Value:

	None.

*/

{
	if (1 == m_PWMChannelId)	{
		PWMCtrlRegs->CTL.PWEN1 = 0;
		PWM1InUse = CHANNEL_NOT_IN_USE;

	} else if (2 == m_PWMChannelId) {
		PWMCtrlRegs->CTL.PWEN2 = 0;
		PWM2InUse = CHANNEL_NOT_IN_USE;
	}

	NumOfPWMInstances -= 1;
	Uninit();
	return;
}

void
GpioPwm::SetPWMCtrl (
	_In_ const PWMRegCTL& CTL
	)

/*
 Routine Description:

	This routine updates the PWM CTL register.

 Parameters:

 	CTL - Supplies the register value to update.

 Return Value:

	None.

*/

{

	PWMCtrlRegs->CTL.word = CTL.word;
	if (1 == m_PWMChannelId) {
		m_UsingFIFO = CTL.USEF1;

	} else if (2 == m_PWMChannelId) {
		m_UsingFIFO = CTL.USEF2;
	}

	usleep(100);
}

void
GpioPwm::SetPWMCtrl (
	_In_ int32_t Mode,
	_In_ int32_t Fifo
	)

/*
 Routine Description:

	This routine updates the PWM CTL register.

 Parameters:

 	Mode - Supplies serial or normal mode.

 	Fifo - Supplies using FIFO or not.

 Return Value:

	None.

*/

{

	PWMRegCTL PWMCtl;
	PWMCtl.word = GetPWMCTL().word;
	if (1 == m_PWMChannelId) {
		PWMCtl.word &= 0xFFFFFF00;
		PWMCtl.MODE1 = Mode;
		PWMCtl.USEF1 = Fifo;
		m_UsingFIFO = Fifo;

	} else if (2 == m_PWMChannelId) {
		PWMCtl.word &= 0xFFFFFF00;
		PWMCtl.MODE2 = Mode;
		PWMCtl.USEF2 = Fifo;
		m_UsingFIFO = Fifo;

	} else {
		RPI_PRINT_EX(InfoLevelError,
					 "PWM channel %d is not supported",
					 m_PWMChannelId);

		assert(false);
	}

	SetPWMCtrl(PWMCtl);
	return;
}

const volatile
GpioPwm::PWMRegCTL&
GpioPwm::GetPWMCTL (
	void
	)

/*
 Routine Description:

	This routine returns the value of CTL register.

 Parameters:

 	None.

 Return Value:

	PWMRegCTL - Value of the CTL register

*/

{

	return PWMCtrlRegs->CTL;
}

const volatile
GpioPwm::PWMRegSTA&
GpioPwm::GetPWMSTA (
	void
	)

/*
 Routine Description:

	This routine returns the value of STA register.

 Parameters:

 	None.

 Return Value:

	PWMRegSTA - Value of the STA register

*/

{

	return PWMCtrlRegs->STA;
}

void
GpioPwm::SetClock (
	_In_ int32_t ClkDivisor
	)

/*
 Routine Description:

	This routine sets the PWM clock.

	Unfortunately, the description to clock manager of BCM2835/2827 is missing
	in the datasheet, so I'm trying to "reverse-engineering" the wiringPi library
	along with information I can find on Google.

 Parameters:

 	ClkDivisor - Supplies the value of divisor.

 Return Value:

	None.

*/

{

	PWMRegCTL pwm_CTL;

	pwm_CTL.word = GetPWMCTL().word;
	ClkDivisor &= 4095;

	//
	//We need to stop the pwm and pwm clock before changing the clock divisor
	//

	PWMCtrlRegs->CTL.word = 0;

	//
	// Some information on the CMPERIICTL: https://elinux.org/BCM2835_registers#CM_PWMCTL
	//

	*CM_PWMCTL = BCM_PASSWORD | 0x01;
	usleep(1000);

	//
	// Wait for clock to be !BUSY
	//

	while ((*CM_PWMCTL & 0x80) != 0) {
		usleep(1);
	}

	*CM_PWMDIV = BCM_PASSWORD | (ClkDivisor << 12);
	*CM_PWMCTL = BCM_PASSWORD | 0x11;				// Start PWM clock
	PWMCtrlRegs->CTL.word = pwm_CTL.word;			// restore PWM_CONTROL
	usleep(1000);
	return;
}

void
GpioPwm::SetRange (
	_In_ uint32_t Range
	)

/*
 Routine Description:

	This routine sets the range register of PWM.

 Parameters:

 	Range - Supplies the value to be set.

 Return Value:

	None.

*/

{

	if (1 == m_PWMChannelId) {
		PWMCtrlRegs->RNG1 = Range;

	} else if (2 == m_PWMChannelId) {
		PWMCtrlRegs->RNG2 = Range;

	} else {
		RPI_PRINT_EX(InfoLevelError,
							 "PWM channel %d is not supported",
							 m_PWMChannelId);

		assert(false);
	}

	usleep(100);
}

void
GpioPwm::PWMOnOff (
	_In_ int32_t Val
	)

/*
 Routine Description:

	This routine turns PWM output on/off.

 Parameters:

 	Val - Supplies turning the output on/off.

 Return Value:

	None.

*/

{
	assert(PWMCtrlRegs != NULL);
	if (1 == m_PWMChannelId) {
		PWMCtrlRegs->CTL.PWEN1 = Val;

	} else if (2 == m_PWMChannelId) {
		PWMCtrlRegs->CTL.PWEN2 = Val;

	} else {
		RPI_PRINT_EX(InfoLevelError,
					 "PWM Channel %d is not supported",
					 m_PWMChannelId);

		assert(false);
	}

	return;
}

void
GpioPwm::ClearFIFO (
	void
	)

/*
 Routine Description:

	This routine clears the FIFO.

 Parameters:

 	None.

 Return Value:

	None.

*/

{

	PWMCtrlRegs->CTL.CLRF1 = 1;
}

void
GpioPwm::UpdatePWMOutput (
	_In_ uint32_t Val
	)

/*
 Routine Description:

	This routine udpates the PWM output.

	Note: If this channel is using the FIFO, there's no effect.

 Parameters:

 	None.

 Return Value:

	None.

*/

{

	if (m_UsingFIFO != 0) {
		RPI_PRINT_EX(InfoLevelWarning,
					 "Channle %d is in FIFO mode!",
					 m_PWMChannelId);
	}

	if (1 == m_PWMChannelId) {
		PWMCtrlRegs->DAT1 = Val;

	} else if (2 == m_PWMChannelId) {
		PWMCtrlRegs->DAT2 = Val;

	} else {
		RPI_PRINT_EX(InfoLevelError,
							 "PWM Channel %d is not supported",
							 m_PWMChannelId);

		assert(false);
	}

	return;
}

void
GpioPwm::UpdatePWMFIFO (
	_In_ uint32_t *vals,
	_In_ uint32_t len
	)

/*
 Routine Description:

	This routine fills the PWM FIFO.

	Note: If this channel is not using the FIFO, there's no effect.

 Parameters:

 	None.

 Return Value:

	None.

*/

{

	if (m_UsingFIFO != 0) {
		RPI_PRINT_EX(InfoLevelWarning,
					 "Channle %d is NOT in FIFO mode!",
					 m_PWMChannelId);
	}

	for (uint32_t i = 0; i < len; ++i) {
		PWMCtrlRegs->FIF1 = vals[i];

		while(GetPWMSTA().FULL1 != 0) {
			usleep(10);
		}
	}
}

