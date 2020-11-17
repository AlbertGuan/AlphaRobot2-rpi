/*
 * GpioClk.h
 *
 *  Created on: May 11, 2019
 *      Author: aobog
 */

#pragma once

#include "GpioBase.h"
#include "Rpi3BConstants.h"

class GpioClk : public GpioBase
{
public:

	GpioClk (
		int32_t Pin,
		float Freq
	);

	virtual
	~GpioClk (
		void
	);

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

	static
	int32_t
	GetChannelFromPin (
		int32_t Pin
	);

	void
	SetFreq (
		float Freq
	);

	void
	StopClock (
		void
	);

	void
	StartClock (
		void
	);

private:

	static const uint32_t GPIO_CLK_PHY_ADDR 	= PERIPHERAL_PHY_BASE + GPIO_CLOCK_OFFSET;
	//They are registers to control clock frequency: https://elinux.org/BCM2835_registers#CM_GP0CTL
	static const uint32_t CM_GPxCTL_OFFSET[3];			//CM_GPxCTL
	static const uint32_t CM_GPxDIV_OFFSET[3];			//CM_GPxDIV

	static volatile uint32_t *clk_base;

	static int32_t num_of_clk_inst;

	volatile uint32_t *CM_GP_CTL;
	volatile uint32_t *CM_GP_DIV;
	int32_t m_Pins;
	float m_Freq;
	int32_t m_channel;

};

void BuzzerTest();
