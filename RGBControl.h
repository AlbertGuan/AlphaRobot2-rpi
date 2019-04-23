/*
 * RGBControl.h
 *
 *  Created on: Apr 14, 2019
 *      Author: aobog
 */

#ifndef RGBCONTROL_H_
#define RGBCONTROL_H_

#include <wiringPi.h>
#include <stdint.h>
#include "AlphaBotTypes.h"
#include "Rpi3BConstants.h"

void RBGControl();

typedef struct
{
	uint32_t select[6];
	const uint32_t rev_0x7E20_0018;
	uint32_t out_set[2];
	const uint32_t rev_0x7E20_0024;
	uint32_t out_clear[2];
	const uint32_t rev_0x7E20_0030;
	uint32_t pin_level[2];
	const uint32_t rev_0x7E20_003C;
	uint32_t event_detect_status[2];
	const uint32_t rev_0x7E20_0048;
	uint32_t rising_edge_detect_enable[2];
	const uint32_t rev_0x7E20_0054;
	uint32_t falling_edge_detect_enable[2];
	const uint32_t rev_0x7E20_0060;
	uint32_t high_detect_enable[2];
	const uint32_t rev_0x7E20_006C;
	uint32_t low_detect_enable[2];
	const uint32_t rev_0x7E20_0078;
	uint32_t async_rising_detect_enable[2];
	const uint32_t rev_0x7E20_0084;
	uint32_t async_falling_detect_enable[2];
	const uint32_t rev_0x7E20_0090;
	uint32_t pull_enable;
	uint32_t pull_enable_clk[2];
	const uint32_t rev_0x7E20_00A0[4];
	uint32_t test;
}gpio_reg_t;

//PWM Control register "CTL Register"
typedef union
{
	struct
	{
		uint32_t PWEN1	: 1;		//Channel 1 enable
		uint32_t MODE1	: 1;		//Channel 1 mode
		uint32_t RPTL1	: 1;		//Channel 1 Repeat Last Data, 0: transmission interrupts when FIFO is empty, 1: last data in FIFO is transmitted repeatedly until FIFO isn't empty
		uint32_t SBIT1	: 1;		//Channel 1 Silence Bit, defines the state of the output when no transmission takes place
		uint32_t POLA1	: 1;		//Channel 1 polarity
		uint32_t USEF1	: 1;		//Channel 1 use FIFO, 0: use PWM_DATx, 1: use FIFO
		uint32_t CLRF1	: 1;		//Clear FIFO
		uint32_t MSEN1	: 1;		//Channel 1 M/S enable

		uint32_t PWEN2	: 1;		//Channel 2 enable
		uint32_t MODE2	: 1;		//Channel 2 mode
		uint32_t RPTL2	: 1;		//Channel 2 Repeat Last Data, 0: transmission interrupts when FIFO is empty, 1: last data in FIFO is transmitted repeatedly until FIFO isn't empty
		uint32_t SBIT2	: 1;		//Channel 2 Silence Bit, defines the state of the output when no transmission takes place
		uint32_t POLA2	: 1;		//Channel 2 polarity
		uint32_t USEF2	: 1;		//Channel 2 use FIFO, 0: use PWM_DATx, 1: use FIFO
		uint32_t 		: 1;		//Reserved
		uint32_t MSEN2	: 1;		//Channel 2 M/S enable
		uint32_t		: 16;
	};
	uint32_t word;
}pwm_reg_CTL_t;

//PWM Status register "STA Register"
typedef union
{
	struct
	{
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
}pwm_reg_STA_t;

//"DMAC Register"
typedef union
{
	struct
	{
		uint32_t DREQ	: 8;		//DMA threshold for DREQ signal
		uint32_t PANIC	: 8;		//DMA threshold for PANIC signal
		uint32_t 		: 15;
		uint32_t ENAB	: 1;		//DMA Enable, 0: DMA disabled, 1: DMA enabled
	};
	uint32_t word;
}pwm_reg_DMAC;

//Refer to "PWM Address Map" for more details
typedef struct
{
	pwm_reg_CTL_t CTL;		//PWM Control
	pwm_reg_STA_t STA;		//PWM Status
	pwm_reg_DMAC DMAC;		//PWM DMA Configuration
	const uint32_t reserve_0;
	uint32_t RNG1;		//PWM channel 1 range
	uint32_t DAT1;		//PWM channel 1 data
	uint32_t FIF1;		//PWM FIFO Input
	const uint32_t reserve_1;
	uint32_t RNG2;		//PWM channel 2 range
	uint32_t DAT2;		//PWM channel 2 data
}pwm_ctrl_t;

/*
 * OI1: what's the relationship between PWM freq and divisor?
 * */
class PWMCtrl
{
public:
	PWMCtrl(int32_t pin, int32_t mode, int32_t range, int32_t clk_div);
	~PWMCtrl();

	void SetMode(uint32_t mode);
	void SetRange(uint32_t range);
	void SetClock(int32_t clk_div);
	void pwmWrite(uint32_t val);


	void PrintAddress();

	typedef enum
	{
		PWM_MODE_BALANCE = 0,
		PWM_MODE_M_S = 1
	}PWMTypes;

private:
	static const uint32_t GPIO_BASE_PHY_ADDR 	= PERIPHERAL_PHY_BASE + GPIO_BASE_OFFSET;
	static const uint32_t GPIO_PWM_PHY_ADDR 	= PERIPHERAL_PHY_BASE + GPIO_PWM_OFFSET;
	static const uint32_t GPIO_CLK_PHY_ADDR 	= PERIPHERAL_PHY_BASE + GPIO_CLOCK_OFFSET;
	static const uint32_t CM_PERIICTL_OFFSET 	= 0x000000A0;
	static const uint32_t CM_PERIIDIV_OFFSET 	= 0x000000A4;
	static const int32_t PWM_CLK_SRC_REQ		= 19200000;		//19.2MHz
	static int32_t mem_fd;

	static volatile gpio_reg_t *gpio_base;
	static volatile pwm_ctrl_t *pwm_base;
	static volatile uint32_t *clk_base;
	static volatile uint32_t *CM_PERIICTL;
	static volatile uint32_t *CM_PERIIDIV;

	static int32_t pwm_1_in_use;
	static int32_t pwm_2_in_use;

	uint32_t m_pwm_channel;
	uint32_t m_range;
};

void PWMTest();
#endif /* RGBCONTROL_H_ */
