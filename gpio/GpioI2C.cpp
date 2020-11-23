/*
 * GpioI2C.cpp
 *
 *  Created on: May 8, 2019
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
#include "GpioI2C.h"

const uint32_t GpioI2C::GPIO_I2C_PHY_ADDR[2] = { PERIPHERAL_PHY_BASE + GPIO_I2C0_OFFSET, PERIPHERAL_PHY_BASE + GPIO_I2C1_OFFSET};
int32_t GpioI2C::NumOfI2CInstances = 0;
int32_t GpioI2C::I2C0InUse = -1;
int32_t GpioI2C::I2C1InUse = -1;
GpioI2C::I2CCtrlRegister GpioI2C::CTRL = { 0 };
GpioI2C::I2CStatusRegister GpioI2C::STATUS = { 0 };

GpioI2C::GpioI2C (
	_In_ int32_t PinSda,
	_In_ int32_t PinScl
) : GpioBase({PinSda, PinScl},
			 GetPinSelection(PinSda, PinScl))

/*
 Routine Description:

	This routine is the constructor of GpioI2C, it is responsible for:
	1. Inits GPIO basic control registers and pin selection (GpioBase)
	2. Maps the GPIO I2C control registers
	3. Reset the Channel and its FIFO

	TODO: The pin selection is hard coded in GetPinSelection for RPI 3B.
	TODO: The channel selection is hard coded in GetChannel for RPI 3B.

 Parameters:

 	PinSda - Supplies the pin number of sda.

 	PinScl - Supplies the pin number of scl.

 Return Value:

	None.

*/

{

	//
	//Step 0: Get channel number
	//

	GetChannel();

	//
	//Step 1: map registers
	//

	try	{
		m_I2CRegisters = const_cast<volatile I2CRegisters *>(static_cast<I2CRegisters *>(
						mmap(NULL,
							 sizeof(I2CRegisters),
							 (PROT_READ | PROT_WRITE),
							 MAP_SHARED,
							 mem_fd,
							 GPIO_I2C_PHY_ADDR[m_I2CChannelId])));

		if (MAP_FAILED == m_I2CRegisters)
			throw "Failed to map m_I2CRegisters channel " + std::to_string(m_I2CChannelId);

	} catch (const std::string &exp) {
		std::cout << "GpioI2C Constructor got exception: " << exp << std::endl;

	} catch (...) {
		std::cout << "GpioI2C Constructor got unknown exception: " << std::endl;
	}

	//
	//Step 2: Disable the channel
	//

	OnOff(OFF);
	usleep(100);

	//
	//Step 3: Clear the FIFO
	//

	ClearFIFO();

	return;
}

GpioI2C::~GpioI2C (
	void
)

/*
 Routine Description:

	This routine is the destructor, it stops the channel and unmap registers.

 Parameters:

 	None.

 Return Value:

	None.

*/

{

	//
	//Disable the I2C
	//

	OnOff(OFF);

	//
	//Clear the FIFO
	//

	ClearFIFO();

	if (m_I2CRegisters) {
		munmap(static_cast<void *>(const_cast<I2CRegisters *>(m_I2CRegisters)), sizeof(I2CRegisters));
		m_I2CRegisters = NULL;
	}

	return;
}

GPIO_FUN_SELECT
GpioI2C::GetPinSelection (
	int32_t sda,
	int32_t scl
)

/*
 Routine Description:

	This routine gets pin selections. It is based on
	CH6.2 of BCM2837 ARM Peripheral.pdf

	TODO: This routine is based on BCM2837 which might not be platform compatible.

 Parameters:

 	sda - Supplies the pin number to sda.

 	scl - Supplies the pin number to sda.

 Return Value:

	int32_t - Supplies the pin selection.

*/

{

	if ((0 == sda) && (1 == scl)) {
		return FSEL_ALT_0;

	} else if ((2 == sda) && (3 == scl)) {
		return FSEL_ALT_0;

	} else if ((28 == sda) && (29 == scl)) {
		return FSEL_ALT_0;

	} else if ((44 == sda) && (45 == scl)) {

		//
		//Note: We don't support using I2C0 on pin 44, 45
		//

		return FSEL_ALT_2;

	} else {
		std::cout << "SDA " << sda << " SCL " << scl << " doesn't support I2C!" << std::endl;
		assert(false);
	}

	return FSEL_INPUT;
}

void
GpioI2C::GetChannel (
	void
)

/*
 Routine Description:

	This routine gets I2C channel ID from pin numbers. It is based on
	CH6.2 of BCM2837 ARM Peripheral.pdf

	TODO: This routine is based on BCM2837 which might not be platform compatible.

 Parameters:

 	None.

 Return Value:

	None.

*/

{

	try
	{

		//
		// Note m_Pins[0] is sda, m_Pins[1] is scl.
		//

		if ((0 == m_Pins[0] && 1 == m_Pins[1]) ||
			(28 == m_Pins[0] && 29 == m_Pins[1])) {

			if (I2C0InUse == -1) {
				m_I2CChannelId = 0;
				I2C0InUse = m_Pins[0];

			} else if (I2C0InUse != m_Pins[0]) {
				throw "Failed to init pin " + std::to_string(m_Pins[0]) + " occupied by " + std::to_string(I2C0InUse);
			}

		} else if ((2 == m_Pins[0] && 3 == m_Pins[1]) ||
				   (44 == m_Pins[0] && 45 == m_Pins[1])) {

			if (I2C1InUse == -1) {
				m_I2CChannelId = 1;
				I2C1InUse = m_Pins[0];

			} else if (I2C1InUse != m_Pins[0]){
				throw "Failed to init pin " + std::to_string(m_Pins[0]) + " occupied by " + std::to_string(I2C1InUse);
			}

		} else {
			throw "Pin " + std::to_string(m_Pins[0]) + " doesn't support I2C";
		}

	} catch (const std::string &exp) {
		std::cout << "GpioPwm::GetChannel() got exception: " << exp << std::endl;

	} catch (...) {
		std::cout << __func__ << " got unknown exception" << std::endl;
	}

	return;
}

void
GpioI2C::UpdateWriteCtrl (
	void
)

/*
 Routine Description:

	This routine updates the control register to write data out.

 Parameters:

 	None.

 Return Value:

	None.

*/

{

	CTRL.word = 0;
	CTRL.I2CEN = 1;
	CTRL.ST = 1;
	CTRL.CLEAR = 1;
	m_I2CRegisters->C.word = CTRL.word;
}

void
GpioI2C::UpdateReadCtrl (
	void
)

/*
 Routine Description:

	This routine updates the control register to read data in.

 Parameters:

 	None.

 Return Value:

	None.

*/

{

	CTRL.word = 0;
	CTRL.I2CEN = 1;
	CTRL.ST = 1;
	CTRL.CLEAR = 1;
	CTRL.READ = 1;
	m_I2CRegisters->C.word = CTRL.word;
}

void
GpioI2C::UpdateStatus (
	void
)

/*
 Routine Description:

	This routine resets the status register.

 Parameters:

 	None.

 Return Value:

	None.

*/

{

	STATUS.word = 0;
	STATUS.CLKT = 1;
	STATUS.ERR = 1;
	STATUS.DONE = 1;
	m_I2CRegisters->S.word = STATUS.word;
}

int16_t
GpioI2C::read (
	_In_ int8_t addr,
	_In_ int8_t reg,
	_Out_ int8_t *vals,
	_In_ const int16_t len
)

/*
 Routine Description:

	This routine reads data from the channel.

 Parameters:

 	addr - ?

 	reg - ?

 	vals - ?

 	len - ?

 Return Value:

	int32_t - Not used, always 0.

*/

{

	int16_t received = 0;

	assert(vals != NULL);
	assert(m_I2CRegisters != NULL);

#if DBG

	printf("GpioI2C::read: 0x%p\n", m_I2CRegisters);

#endif

	if (1 == write(addr, reg)) {

#if DBG

		printf("Assign Address: %d\n", addr);

#endif

		m_I2CRegisters->A.ADDR = addr;

#if DBG

		printf("Assign Length: %d\n", len);

#endif
		m_I2CRegisters->DLEN.DLEN = len;
		UpdateReadCtrl();
		UpdateStatus();

		while (received < len) {
			while (m_I2CRegisters->S.RXD && received < len) {
				vals[received] = m_I2CRegisters->FIFO;
				++received;
			}
		}

#if DBG

		printf("Got all\n");

#endif

		while (0 == m_I2CRegisters->S.DONE);
	}

	return 0;
}

int16_t
GpioI2C::write (
	_In_ int8_t addr
	)

/*
 Routine Description:

	This routine writes nothing to the channel. It resets the channel.

 Parameters:

 	addr - ?

 Return Value:

	int32_t - Number of bytes wrote to the channel.

*/

{

	return write(addr, {});
}

int16_t
GpioI2C::write (
	_In_ int8_t addr,
	_In_ int8_t val
	)

/*
 Routine Description:

	This routine writes a byte to the channel.

 Parameters:

 	addr - ?

 	val - Value to write

 Return Value:

	int32_t - Number of ? wrote to the channel.

*/

{

	return write(addr, std::vector<int8_t>{val});
}

int16_t
GpioI2C::write (
	_In_ int8_t addr,
	_In_ const std::vector<int8_t> &vals
	)

/*
 Routine Description:

	This routine writes a vector of bytes to the channel.

 Parameters:

 	addr - ?

 	vals - Supplies a vector of data to write.

 Return Value:

	int32_t - Number of bytes wrote to the channel.

*/

{

	int16_t len = vals.size();
	int16_t sent = 0;

	m_I2CRegisters->A.ADDR = addr;
	m_I2CRegisters->DLEN.DLEN = len;
	std::cout << "I2C write to " << static_cast<int32_t>(addr) << " " << len << " bytes of data\n";
	UpdateStatus();
	UpdateWriteCtrl();
	while (sent < len) {
		if (1 == m_I2CRegisters->S.TXD) {
			m_I2CRegisters->FIFO = vals[sent];
			sent += 1;
		}
	}

#ifdef DBG

	std::cout << sent << " bytes of data written to  FIFO\n";

#endif

	while (0 == m_I2CRegisters->S.DONE);

#ifdef DBG

	std::cout << "I2C write finished " << sent << " bytes of data sent" << std::endl;

#endif

	return sent;
}

void
GpioI2C::OnOff (
	_In_ int32_t val
	)

/*
 Routine Description:

	This routine turns the channel on/off.

 Parameters:

 	val - Value to write

 Return Value:

	None.

*/

{
	m_I2CRegisters->C.I2CEN = val;
}

void
GpioI2C::ClearFIFO (
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
	m_I2CRegisters->C.CLEAR = 1;
}

