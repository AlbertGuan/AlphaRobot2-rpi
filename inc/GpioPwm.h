/*
 * PWM.h
 *
 *  Created on: Apr 23, 2019
 *      Author: Albert Guan
 */

#pragma once
#include <wiringPi.h>
#include <stdint.h>
#include <stddef.h>     /* offsetof */
#include "AlphaBotTypes.h"
#include "GpioBase.h"
#include "MemBase.h"
#include "Rpi3BConstants.h"

/*
 * OI1: what's the relationship between PWM freq and divisor?
 * Answer: It depends on the mode MODE1/2 of CTL register
 * 		   1. PWM Mode: the PWM freq = 19.2MHz / divisor / RNG, e.g. To control the SG90 servo motor, which requires 50Hz,
 * 		      we pick divisor = 1024 (Note the divisor cannot be larger than 4095), and range 375, the PWM freq = 19.2MHz / 1024 / 375 = 50Hz
 * 		   2. Serializer Mode: The PWM controller reads one bit from the PWM data/fifo every (divisor/19.2)ns and put it on the output pin.
 * */
class GpioPwm : public GpioBase
{
public:

	//
	// PWM Control register "CTL Register"
	// Check CH9.6 Control and Status Registers for more details
	//

	typedef union _PWMRegCTL_ {
		struct {

			//
			// Channel 1 enable
			//

			uint32_t PWEN1	: 1;

			//
			// Channel 1 mode
			// 1: Serialized mode, 0: Normal mode. Check CH9.3 and 9.4 for more details.
			//

			uint32_t MODE1	: 1;

			//
			// Channel 1 Repeat Last Data,
			// 0: transmission interrupts when FIFO is empty
			// 1: last data in FIFO is transmitted repeatedly until FIFO isn't empty
			//

			uint32_t RPTL1	: 1;

			//
			// Channel 1 Silence Bit, defines the state of the output when no
			// transmission takes place
			//

			uint32_t SBIT1	: 1;

			//
			// Channel 1 polarity
			//

			uint32_t POLA1	: 1;

			//
			// Channel 1 use FIFO, 0: use PWM_DATx, 1: use FIFO
			//

			uint32_t USEF1	: 1;

			//
			// Clear FIFO
			//

			uint32_t CLRF1	: 1;

			//
			// Channel 1 M/S enable, check CH9.3 and 9.4 for more details.
			// For example, if I would like to send a 20% duty cycle PWM wave,
			// and the PWM frequency is 1Hz, the PWM clock is 10Hz. The serialized
			// mode will send a wave like this:
			//              0123456789
			//              ___
			// Disabled:   |   |_______
			//              _     _
			// Enabled:    | |___| |___
			//

			uint32_t MSEN1	: 1;

			//
			// Channel 2 control bits
			//

			uint32_t PWEN2	: 1;
			uint32_t MODE2	: 1;
			uint32_t RPTL2	: 1;
			uint32_t SBIT2	: 1;
			uint32_t POLA2	: 1;
			uint32_t USEF2	: 1;
			uint32_t 		: 1;
			uint32_t MSEN2	: 1;
			uint32_t		: 16;
		};

		uint32_t word;
	} PWMRegCTL, *PPWMRegCTL;

	//
	// PWM Status register "STA Register"
	// Check CH9.6 Control and Status Registers for more details
	//

	typedef union _PWMRegSTA_ {
		struct {
			uint32_t FULL1	: 1;		//FIFO Full Flag
			uint32_t EMPT1	: 1;		//FIFO Empty Flag
			uint32_t WERR1	: 1;		//FIFO Write Error Flag
			uint32_t RERR1	: 1;		//FIFO Read Error Flag
			uint32_t GAPO1	: 1;		//Channel 1 Gap Occurred Flag, indicates that there has been a gap between transmission of two consecutive data from FIFO
			uint32_t GAPO2	: 1;		//Channel 2 Gap Occurred Flag
			uint32_t GAPO3	: 1;		//Channel 3 Gap Occurred Flag
			uint32_t GAPO4	: 1;		//Channel 4 Gap Occurred Flag
			uint32_t BERR	: 1;		//Bus Error Flag
			uint32_t STA1	: 1;		//Channel 1 State, 0: the channel is not currently transmitting, 1: is transmitting
			uint32_t STA2	: 1;		//Channel 2 State, 0: the channel is not currently transmitting, 1: is transmitting
			uint32_t STA3	: 1;		//Channel 3 State, 0: the channel is not currently transmitting, 1: is transmitting
			uint32_t STA4	: 1;		//Channel 4 State, 0: the channel is not currently transmitting, 1: is transmitting
			uint32_t		: 19;
		};

		uint32_t word;
	} PWMRegSTA, *PPWMRegSTA;

	//
	// PWM DMA control registers
	// Check CH9.6 Control and Status Registers for more details
	//

	typedef union _PWMRegDMAC_ {
		struct {
			uint32_t DREQ	: 8;		//DMA threshold for DREQ signal
			uint32_t PANIC	: 8;		//DMA threshold for PANIC signal
			uint32_t 		: 15;
			uint32_t ENAB	: 1;		//DMA Enable, 0: DMA disabled, 1: DMA enabled
		};

		uint32_t word;
	} PWMRegDMAC, *PPWMRegDMAC;

	//
	//Refer to "PWM Address Map" for more details
	//

	typedef struct _PWMCtrlRegisters_ {
		PWMRegCTL CTL;		//PWM Control
		PWMRegSTA STA;		//PWM Status
		PWMRegDMAC DMAC;	//PWM DMA Configuration
		const uint32_t reserve_0;
		uint32_t RNG1;		//PWM channel 1 range
		uint32_t DAT1;		//PWM channel 1 data
		uint32_t FIF1;		//PWM FIFO Input
		const uint32_t reserve_1;
		uint32_t RNG2;		//PWM channel 2 range
		uint32_t DAT2;		//PWM channel 2 data
	} PWMCtrlRegisters, *PPWMCtrlRegisters;

	static int32_t Init();
	static int32_t Uninit();
	static GPIO_FUN_SELECT GetPinSelection(uint32_t pin);
	int32_t GetChannel();

	//
	// Init and configuration routines
	//

	GpioPwm (
		_In_ int32_t Pin,
		_In_ int32_t Range,
		_In_ int32_t Divisor,
		_In_ int32_t Mode,
		_In_ int32_t Fifo
		);

	~GpioPwm();

	void
	SetPWMCtrl (
		_In_ const PWMRegCTL&
		);

	void
	SetPWMCtrl (
		_In_ int32_t Mode,
		_In_ int32_t Fifo
		);

	const volatile
	PWMRegCTL&
	GetPWMCTL(
		void
		);

	const volatile
	PWMRegSTA&
	GetPWMSTA(
			void
			);

	void
	SetClock (
		_In_ int32_t ClkDivisor
		);

	void
	SetRange (
		_In_ uint32_t range
		);

	void
	PWMOnOff (
		_In_ int32_t Val
		);

	void
	ClearFIFO (
		void
		);

	//
	// PWM operation routines
	//

	void
	UpdatePWMOutput (
		_In_ uint32_t Val
		);

	void
	UpdatePWMFIFO (
		uint32_t *vals,
		uint32_t len
		);

private:
	static const uint32_t GPIO_PWM_PHY_ADDR 	= PERIPHERAL_PHY_BASE + GPIO_PWM_OFFSET;
	static const uint32_t GPIO_CLK_PHY_ADDR 	= PERIPHERAL_PHY_BASE + GPIO_CLOCK_OFFSET;
	static const uint32_t PWM_FIFO_PHY_ADDR		= GPIO_PWM_PHY_ADDR + offsetof(PWMCtrlRegisters, FIF1);
	static const uint32_t PWM_FIFO_BUS_ADDR		= PERIPHERAL_BUS_BASE + GPIO_PWM_OFFSET + offsetof(PWMCtrlRegisters, FIF1);
	//They are registers to control the pwm frequency, https://elinux.org/BCM2835_registers#CM_PWMCTL
	static const uint32_t CM_PWMCTL_OFFSET 	= 0x000000A0;			//CM_PWMCTL
	static const uint32_t CM_PWMDIV_OFFSET 	= 0x000000A4;			//CM_PWMDIV

	static volatile PWMCtrlRegisters *PWMCtrlRegs;
	static volatile uint32_t *ClkRegisters;
	static volatile uint32_t *CM_PWMCTL;
	static volatile uint32_t *CM_PWMDIV;

	static int32_t NumOfPWMInstances;
	static int32_t PWM1InUse;
	static int32_t PWM2InUse;

	int32_t m_PWMChannelId;
	uint32_t m_Range;
	uint32_t m_UsingFIFO;
};
