/*
 * GPIO.cpp
 *
 *  Created on: May 7, 2019
 *      Author: aguan
 */

#include <iostream>
#include <iomanip>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <bitset>
#include <string>
#include "GpioBase.h"

volatile GpioBase::PGPIORegisters GpioBase::GPIORegs = NULL;
int32_t GpioBase::num_of_gpio_inst = 0;

GpioBase::GpioBase (
	const std::vector<int32_t> &Pins,
	GPIO_FUN_SELECT PinSelect
) : m_Pins(Pins),
	m_PinSelection(PinSelect)

/*
 Routine Description:

	This routine is the constructor of GpioBase, it maps the GPIO basic control
	registers (for the first instance). Then set pin selection registers.

 Parameters:

 	Pins - Supplies a vector of pins to set.

 	PinSelect - Supplies which ALT to set for Pins.

 Return Value:

	None.

*/

{

	//
	// Map the GPIO control registers.
	//

	Init();

	//
	// Set pin selection registers.
	//

	SetPinSelection();
	return;
}

GpioBase::~GpioBase (
	void
)

/*
 Routine Description:

	This routine is the destructor of GpioBase, it unmapps the GPIO control
	registers (for the last instance).

 Parameters:

 	None.

 Return Value:

	None.

*/

{

	//
	// Unmap GPIO registers.
	//

	Uninit();
	return;
}

void
GpioBase::SetPinSelection(
	void
)

/*
 Routine Description:

	This routine sets GPIO pin selection registers.

 Parameters:

 	None.

 Return Value:

	None.

*/

{

	uint32_t bit_off;
	uint32_t current;
	uint32_t word_off;

	for (auto pin : m_Pins) {

		//
		//Each GPIO selection word contains 10 pins.
		//

		word_off = pin / 10;

		//
		//Each pin takes 3 bits in GPIO selection word.
		//

		bit_off = pin % 10 * 3;

		//
		// Snap the current value.
		//

		current = GPIORegs->GPFSELn[word_off];

#ifdef DBG

		std::bitset<32> before(current);

#endif

		current &= ~(0x7 << bit_off);
		current |= m_PinSelection << bit_off;
		GPIORegs->GPFSELn[word_off] = current;

#ifdef DBG

		std::bitset<32> after(current);
		std::cout << __func__ << std::endl;
		std::cout << "Before: " << before << std::endl;
		std::cout << "After:  " << after << std::endl;

#endif

	}

	return;
}

int32_t
GpioBase::Init (
	void
)

/*
 Routine Description:

	This routine maps GPIO registers.

 Parameters:

 	None.

 Return Value:

	int32_t - TODO: Have an error code map when we have more errors.

*/

{

	//
	// Only map the memory once if it's the first instance.
	//

	if (0 == num_of_gpio_inst)
	{
		//
		//Check whether the "/dev/mem" has been opened or not
		//

		if (mem_fd != 0) {
			try
			{
				GPIORegs = (GPIORegisters *) mmap(NULL,
												  sizeof(GPIORegisters),
												  (PROT_READ | PROT_WRITE),
												  MAP_SHARED,
												  mem_fd,
												  GPIO_BASE_PHY_ADDR);

				if (MAP_FAILED == GPIORegs) {
					throw "Failed to mmap GPIORegs";
				}

			} catch (const std::string &err) {
				std::cout << __func__ << " with exception: " << err << std::endl;
				return -1;

			} catch (...) {
				std::cout << __func__ << " with unknown exception" << std::endl;
				std::cout << "Please try to run with sudo" << std::endl;
				return -2;
			}

		} else {
			return -3;
		}
	}

	//
	// Increase the counter by one if succeed.
	//

	num_of_gpio_inst += 1;
	return 0;
}

void
GpioBase::Uninit (
	void
)

/*
 Routine Description:

	This routine unmaps GPIO registers.

 Parameters:

 	None.

 Return Value:

	None.

*/

{

	num_of_gpio_inst -= 1;
	if ((0 == num_of_gpio_inst) && (GPIORegs != NULL)) {

		//
		//Only const_cast can add/remove the "volatile" keyword
		//

		munmap(static_cast<void *>(const_cast<GPIORegisters *>(GPIORegs)),
			   sizeof(GPIORegisters));

		GPIORegs = NULL;
	}

	return;
}
