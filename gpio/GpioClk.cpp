/*
 * GpioClk.cpp
 *
 *  Created on: May 11, 2019
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
#include "GpioClk.h"
#include "AlphaBotTypes.h"

const uint32_t GpioClk::CM_GPxCTL_OFFSET[3] = { 0x70, 0x78, 0x80 };			//CM_GPxCTL
const uint32_t GpioClk::CM_GPxDIV_OFFSET[3] = { 0x74, 0x7C, 0x84 };			//CM_GPxDIV
volatile uint32_t *GpioClk::ClkRegisters = NULL;
int32_t GpioClk::NumOfClkInstances = 0;

GpioClk::GpioClk(
	int32_t Pin,
	float Freq
) : GpioBase({Pin}, FSEL_ALT_0),
	m_Pins(Pin)

/*
 Routine Description:

	This routine is the constructor of GpioClk, it is responsible for:
	1. Inits GPIO basic control registers and pin selection (GpioBase)
	2. Maps the GPIO clock control registers
	3. Set the frequency and start the clock output

	TODO: The pin selection is hard coded to FSEL_ALT_0 because all clock pins
		on RPI 3B are ALT_0.

 Parameters:

 	Pin - Supplies the pin number to update

 	PinSelect - Supplies which ALT to set for Pins.

 Return Value:

	None.

*/

{

	//
	// Map clock control registers.
	//

	Init();

	//
	// Get the clock channel from pin number.
	//

	m_channel = GetChannelFromPin(Pin);

	//
	// Init clk control and freq registers
	//

	CM_GP_CTL = ADDRESS_TO_VOLATILE_POINTER((uint32_t)ClkRegisters + CM_GPxCTL_OFFSET[m_channel]);
	CM_GP_DIV = ADDRESS_TO_VOLATILE_POINTER((uint32_t)ClkRegisters + CM_GPxDIV_OFFSET[m_channel]);
	SetFreq(Freq);
	return;
}

void
GpioClk::SetFreq (
	float Freq
)

/*
 Routine Description:

	This routine sets the frequency of the clk output.

 Parameters:

 	Freq - Supplies the frequeNcy in Hz.

 Return Value:

	None.

*/

{

	assert(Freq >= 0.0 && Freq <= RPI_OSCILLATOR_FREQ);

	m_Freq = Freq;
	uint32_t divi = RPI_OSCILLATOR_FREQ / Freq;
	uint32_t divr = RPI_OSCILLATOR_FREQ % static_cast<uint32_t>(Freq);
	uint32_t divf = static_cast<uint32_t>(static_cast<float>(divr) * 4096.0 / RPI_OSCILLATOR_FREQ);

	std::cout << "SetFreq: freq: " << Freq << std::endl;
	if (divi > 4095) {
		divi = 4095;
	}

	//
	//Stop the clk
	//

	StopClock();

	//
	//Set dividers
	//

	*CM_GP_DIV = BCM_PASSWORD | (divi << 12) | divf;

	//
	//Start the clock
	//

	StartClock();
	return;
}

GpioClk::~GpioClk (
	void
)

/*
 Routine Description:

	This routine is the destructor.

 Parameters:

 	None.

 Return Value:

	None.

*/

{

	//
	// Stop the clock.
	//

	StopClock();

	//
	// Unmap registers.
	//

	Uninit();
	return;
}

void
GpioClk::StartClock (
	void
)

/*
 Routine Description:

	This routine starts the clock.

 Parameters:

 	None.

 Return Value:

	None.

*/

{

	//
	//Start the clock
	//

	*CM_GP_CTL = BCM_PASSWORD | 0x10 | 0x1;
	return;
}

void
GpioClk::StopClock (
	void
)

/*
 Routine Description:

	This routine stops the clock.

 Parameters:

 	None.

 Return Value:

	None.

*/

{
	//
	//Using the oscillator as the clock source and stop the clock
	//

	*CM_GP_CTL = BCM_PASSWORD | 0x1;
	while (*CM_GP_CTL & 0x80);
	return;
}

int32_t
GpioClk::GetChannelFromPin (
	int32_t Pin
)

/*
 Routine Description:

	This routine converts GPIO pin number to CLK channel ID. It is based on
	CH6.1 of BCM2837 ARM Peripheral.pdf

	TODO: This routine is based on BCM2837 which might not be platform compatible.

 Parameters:

 	Pin - Supplies the pin number to update.

 Return Value:

	int32_t - Supplies the clk channel number.

*/

{
	switch(Pin)	{
		case 4:
			return 0;
		case 5:
			return 1;
		case 6:
			return 2;
		default:
			std::cout << "GpioClk: unsupported pin " << Pin << std::endl;
	}

	return 0;
}

int32_t
GpioClk::Init (
	void
)

/*
 Routine Description:

	This routine maps the clock registers.

 Parameters:

 	None.

 Return Value:

	int32_t - TODO: we are not checking error code now.

*/

{

	if (0 == NumOfClkInstances) {
		try {
			ClkRegisters = const_cast<volatile uint32_t *>(static_cast<uint32_t *>(
							mmap(NULL,
								 BLOCK_SIZE,
								 PROT_READ | PROT_WRITE,
								 MAP_SHARED,
								 mem_fd,
								 GPIO_CLK_PHY_ADDR)));

			if (MAP_FAILED == ClkRegisters) {
				throw "Failed to mmap ClkRegisters";
			}

		} catch (const std::string &exp) {
			std::cout << "GpioPwm Constructor got exception: " << exp << std::endl;
			return -1;

		} catch (...) {
			std::cout << __func__ << " got unknown exception" << std::endl;
			return -2;
		}
	}

	NumOfClkInstances += 1;
	return 0;
}

void
GpioClk::Uninit (
	void
)

/*
 Routine Description:

	This routine unmaps clock registers.

 Parameters:

 	None.

 Return Value:

	None.

*/

{

	NumOfClkInstances -= 1;
	if ((0 == NumOfClkInstances) && (NULL != ClkRegisters)) {
		munmap(static_cast<void *>(const_cast<uint32_t *>(ClkRegisters)), BLOCK_SIZE);
		ClkRegisters = NULL;
	}

	return;
}

void
BuzzerTest (
	void
)

/*
 Routine Description:

	This routine is the test function to ring the buzzer.

 Parameters:

 	None.

 Return Value:

	None.

*/

{
	GpioClk buzzer(4, 200);
	float freq = 200.0;
	int incre = 1000;
	while (1) {
		buzzer.SetFreq(freq);
		freq += incre;
		if (freq >= 10000) {
			incre = -1000;

		} else if (freq < 1000) {
			incre = 1000;
		}

		usleep(100000);
	}

	return;
}
