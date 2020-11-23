/*
 * GpioI2C.h
 *
 *  Created on: May 8, 2019
 *      Author: Albert Guan
 */
#pragma once

#include "GpioBase.h"

class GpioI2C : public GpioBase
{
public:

	//
	// This class implements I2C operations. More details on I2C are described in Ch3 BSC
	// of the BCM2837 ARM-Peripherals.pdf. There are two I2C channels available to the user.
	//

	//
	// I2C control register (C register in the datasheet)
	//

	typedef union _I2CCtrlRegister_{
		struct {
			uint32_t READ		: 1;		//0: TX, 1: RX
			uint32_t res		: 3;
			uint32_t CLEAR		: 2;		//Clear FIFO
			uint32_t res1		: 1;
			uint32_t ST			: 1;		//Start transfer, one shot operation
			uint32_t INTD		: 1;		//INT on Done, See Status register for "Done"
			uint32_t INTT		: 1;		//INT on TX
			uint32_t INTR		: 1;		//INT on RX
			uint32_t res2		: 4;
			uint32_t I2CEN		: 1;		//I2C Enable
			uint32_t res3		: 16;
		};

		uint32_t word;
	}I2CCtrlRegister, *PI2CCtrlRegister;

	//
	// I2C status register
	//

	typedef union _I2CStatusRegister_ {
		struct {
			uint32_t TA			: 1;		//0: TX not active, 1: TX active
			uint32_t DONE		: 1;		//0: TX not completed, 1: TX completed
			uint32_t TXW		: 1;		//0: FIFO is full and a write is underway, 1: FIFO is not full and a write is underway
			uint32_t RXR		: 1;		//0: FIFO is full and a read is underway, 1: FIFO is not full and a read is underway
			uint32_t TXD		: 1;		//1: FIFO can accept data
			uint32_t RXD		: 1;		//1: FIFO has data to read
			uint32_t TXE		: 1;		//1: FIFO is empty, not data to TX
			uint32_t RXE		: 1;		//1: FIFO is full, not further data can be received
			uint32_t ERR		: 1;		//1: Slave has not acknowledged its address, writing 1 to clear
			uint32_t CLKT		: 1;		//1: Slave has held the SCL signal low (clock stretching) for longer than I2CCLKT specified
			uint32_t res		: 22;
		};

		uint32_t word;
	}I2CStatusRegister, *PI2CStatusRegister;

	//
	// Data length register
	//

	typedef union _I2CLenRegister_ {
		struct {
			uint32_t DLEN		: 16;
			uint32_t res		: 16;
		};

		uint32_t word;
	}I2CLenRegister, *PI2CLenRegister;

	//
	// Slave address register
	//

	typedef union _I2CSlaveAddrRegister_ {
		struct {
			uint32_t ADDR		: 7;
			uint32_t res		: 25;
		};

		uint32_t word;
	}I2CSlaveAddrRegister, *PI2CSlaveAddrRegister;

	//
	// Clock divider register
	//

	typedef union _I2CDIVRegister_ {
		struct {
			uint32_t CDIV		: 16;
			uint32_t res		: 16;
		};

		uint32_t word;
	}I2CDIVRegister, *PI2CDIVRegister;

	//
	// Delay register
	//

	typedef union I2CDELRegister
	{
		struct
		{
			uint32_t REDL		: 16;
			uint32_t FEDL		: 16;
		};
		uint32_t word;
	}I2CDELRegister, *PI2CDELRegister;

	//
	// Clock timeout register
	//

	typedef union _I2CCLKTRegister_ {
		struct {
			uint32_t TOUT		: 16;
			uint32_t res		: 16;
		};

		uint32_t word;
	}I2CCLKTRegister, *PI2CCLKTRegister;

	typedef struct _I2CRegisters_ {
		I2CCtrlRegister C;				//Control
		I2CStatusRegister S;			//Status
		I2CLenRegister DLEN;			//Data Length
		I2CSlaveAddrRegister A;			//Address
		uint32_t FIFO;					//FIFO, Do NOT use union, or it fails while writing multiple values
		I2CDIVRegister DIV;				//Clock Divider
		I2CDELRegister DEL;				//Data Delay
		I2CCLKTRegister CLKT;			//Clock Stretch Timeout
	}I2CRegisters, *PI2CRegisters;

	GpioI2C (
		int32_t PinSda,
		int32_t PinScl
	);

	virtual
	~GpioI2C (
		void
	);

	static
	GPIO_FUN_SELECT
	GetPinSelection (
		int32_t sda,
		int32_t scl
	);

	void
	GetChannel (
		void
	);

	int16_t
	read (
		const int8_t addr,
		const int8_t reg,
		int8_t *vals,
		int16_t len = 1
	);

	int16_t
	write (
		const int8_t addr
	);

	int16_t
	write (
		const int8_t addr,
		int8_t val
	);

	int16_t
	write (
		const int8_t addr,
		const std::vector<int8_t> &vals
	);

	void
	UpdateWriteCtrl (
		void
	);

	void
	UpdateReadCtrl (
		void
	);

	void
	UpdateStatus (
		void
	);

	void
	OnOff (
		int32_t val
	);

	void
	ClearFIFO (
		void
	);

protected:
	static const uint32_t GPIO_I2C_PHY_ADDR[2];
	static int32_t NumOfI2CInstances;
	static int32_t I2C0InUse;
	static int32_t I2C1InUse;

	static I2CCtrlRegister CTRL;
	static I2CStatusRegister STATUS;
private:
	volatile I2CRegisters *m_I2CRegisters;
	int32_t m_I2CChannelId;
};
