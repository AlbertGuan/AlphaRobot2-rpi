/*
 * GPIO.h
 *
 *  Created on: May 6, 2019
 *      Author: aobog
 */

#pragma once

#include <vector>
#include "Rpi3BConstants.h"
#include "MemBase.h"

class GpioBase : public MemBase
{
public:

	//
	// GPIORegisters is the mater memory map of BCM2837 GPIO registers.
	// The detailed map can be found in Chapter 6.1 Register View of the
	// BCM2837-ARM-Peripherals.pdf.
	// TODO: They might be not platform compatible, think of moving them to
	//	a header.
	//

	typedef struct _GPIORegisters_ {

		//
		// GPFSELn supplies pin selection registers for GPIO pins. Check GPIO
		// Function Select Registers (GPFSELn) of datasheet for more details.
		//

		uint32_t GPFSELn[6];
		const uint32_t rev_0x7E20_0018;

		//
		// If a pin is set as output pin, GPSET and GPCLEAR set/clear corresponding
		// pin when their bit is set.
		//

		uint32_t GPSETn[2];
		const uint32_t rev_0x7E20_0024;
		uint32_t GPCLRn[2];
		const uint32_t rev_0x7E20_0030;

		//
		// If a pin is set as input pin, GPLEV indicates the input level of the pin.
		//

		uint32_t GPLEVn[2];
		const uint32_t rev_0x7E20_003C;

		//
		// Input pin also supports event detection, e.g. detect a rising edge.
		// Status Register
		// 		GPEDSn: Event detect status register, bit is set when event is detected.
		// Control Registers
		//		GPRENn: Rising edge enable
		//		GPFENn: Falling edge enable
		//		GPHENn: High detect enable
		//		GPLENn: Low detect enable
		//		GPARENn: Async rising edge enable
		//		GPAFENn: Async falling edge enable
		//

		uint32_t GPEDSn[2];
		const uint32_t rev_0x7E20_0048;
		uint32_t GPRENn[2];
		const uint32_t rev_0x7E20_0054;
		uint32_t GPFENn[2];
		const uint32_t rev_0x7E20_0060;
		uint32_t GPHENn[2];
		const uint32_t rev_0x7E20_006C;
		uint32_t GPLENn[2];
		const uint32_t rev_0x7E20_0078;
		uint32_t GPARENn[2];
		const uint32_t rev_0x7E20_0084;
		uint32_t GPAFENn[2];
		const uint32_t rev_0x7E20_0090;

		//
		// These registers control pull up/down of pins.
		//

		uint32_t GPPUD;
		uint32_t GPPUDCLKn[2];
		const uint32_t rev_0x7E20_00A0[4];
	} GPIORegisters, *PGPIORegisters;

	GpioBase (
		const std::vector<int32_t> &Pins,
		GPIO_FUN_SELECT PinSelect = FSEL_INPUT
	);

	virtual
	~GpioBase (
		void
	);

//	virtual void DumpRegisters();
	void
	SetPinSelection (
		void
	);

protected:

	//
	// GPIORegs points to the mapped virtual address of GPIO control registers.
	//

	static volatile PGPIORegisters GPIORegs;

	//
	// Physical address of the GPIO control registers.
	//

	const static uint32_t GPIO_BASE_PHY_ADDR = PERIPHERAL_PHY_BASE + GPIO_BASE_OFFSET;

	static
	int32_t
	Init (
		void
	);

	static
	void
	Uninit (
		void
	);

	//
	// Number of the GPIOxx instances, call Init() for the first instance,
	// and call Unit() for the last instance.
	//

	static int32_t num_of_gpio_inst;

	//
	// This class supports multiple pins are set with the same functionality,
	// then we can update their values all at once.
	// For example, we could set pin 0, 1, 2 to high at the same time by writing
	// bitmask to GPSETn at once.
	//

	std::vector<int32_t> m_Pins;
	GPIO_FUN_SELECT m_PinSelection;
};
