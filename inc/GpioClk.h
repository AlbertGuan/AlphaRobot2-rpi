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
	GpioClk(int32_t pin, float freq);
	virtual ~GpioClk();
	static void Init();
	static void Uninit();
	static int32_t GetChannel(int32_t pin);

	void SetFreq(float freq);
	void Stop();
	void Start();
private:
	static const uint32_t GPIO_CLK_PHY_ADDR 	= PERIPHERAL_PHY_BASE + GPIO_CLOCK_OFFSET;
	//They are registers to control the pwm frequency, https://elinux.org/BCM2835_registers#CM_PWMCTL
	static const uint32_t CM_GPxCTL_OFFSET[3];			//CM_PWMCTL
	static const uint32_t CM_GPxDIV_OFFSET[3];			//CM_PWMDIV

	static volatile uint32_t *clk_base;

	static int32_t num_of_clk_inst;

	volatile uint32_t *CM_GP_CTL;
	volatile uint32_t *CM_GP_DIV;
	int32_t m_Pins;
	float m_freq;
	int32_t m_channel;

};

void BuzzerTest();
