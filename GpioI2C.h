/*
 * GpioI2C.h
 *
 *  Created on: May 8, 2019
 *      Author: aguan
 */
#pragma once

#include "GpioBase.h"

typedef union
{
	struct
	{
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
}I2CCtrlReg_t;

typedef union
{
	struct
	{
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
}I2CStatusReg_t;

typedef union
{
	struct
	{
		uint32_t ADDR		: 7;
		uint32_t res		: 25;
	};
	uint32_t word;
}I2CAddrReg_t;

typedef union
{
	struct
	{
		uint32_t DLEN		: 16;
		uint32_t res		: 16;
	};
	uint32_t word;
}I2CLenReg_t;

typedef union
{
	struct
	{
		uint32_t DATA		: 8;
		uint32_t res		: 24;
	};
	uint32_t word;
}I2CFIFOReg_t;

typedef union
{
	struct
	{
		uint32_t CDIV		: 16;
		uint32_t res		: 16;
	};
	uint32_t word;
}I2CDIVReg_t;

typedef union
{
	struct
	{
		uint32_t REDL		: 16;
		uint32_t FEDL		: 16;
	};
	uint32_t word;
}I2CDELReg_t;

typedef union
{
	struct
	{
		uint32_t TOUT		: 16;
		uint32_t res		: 16;
	};
	uint32_t word;
}I2CCLKTReg_t;

typedef struct
{
	I2CCtrlReg_t C;				//Control
	I2CStatusReg_t S;			//Status
	I2CLenReg_t DLEN;			//Data Length
	I2CAddrReg_t A;				//Address
	I2CFIFOReg_t FIFO;			//FIFO
	I2CDIVReg_t DIV;			//Clock Divider
	I2CDELReg_t DEL;			//Data Delay
	I2CCLKTReg_t CLKT;			//Clock Stretch Timeout
}I2CReg_t;

class GpioI2C : public GpioBase
{
public:
	GpioI2C(int32_t pin_sda, int32_t pin_scl);
	virtual ~GpioI2C();

	static PinSel_t getPinSel(int32_t sda, int32_t scl);
	void getChannel();

	int16_t read(const int8_t addr, const int8_t reg, int8_t *vals, int16_t len = 1);
	int16_t write(const int8_t addr);
	int16_t write(const int8_t addr, int8_t val);
	int16_t write(const int8_t addr, const std::vector<int8_t> &vals);
	void UpdateWriteCtrl();
	void UpdateReadCtrl();
	void UpdateStatus();
	void OnOff(int32_t val);
	void ClearFIFO();
protected:
	static const uint32_t GPIO_I2C_PHY_ADDR[2];

	static int32_t num_of_i2c_inst;
	static int32_t i2c_0_in_use;
	static int32_t i2c_1_in_use;

	static I2CCtrlReg_t CTRL;
	static I2CStatusReg_t STATUS;
private:
	volatile I2CReg_t *m_i2c_base;
	int32_t m_i2c_channel;
};
