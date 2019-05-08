/*
 * Rpi3BConstants.h
 *
 *  Created on: Apr 21, 2019
 *      Author: aobog
 */

#ifndef RPI3BCONSTANTS_H_
#define RPI3BCONSTANTS_H_

#define BLOCK_SIZE					4096

#define PERIPHERAL_PHY_BASE			0x3F000000

//PWM Related Address
#define GPIO_CLOCK_OFFSET			0x00101000
#define GPIO_BASE_OFFSET			0x00200000
#define GPIO_PWM_OFFSET				0x0020C000

//DMA Related Address
// RPI2 and 3 use a different chipset, and the peripheral addresses have changed.
#define DMA_OFFSET					0x00007000

#define GPIO_BASE_BUS				0x7E200000 //this is the physical bus address of the GPIO module. This is only used when other peripherals directly connected to the bus (like DMA) need to read/write the GPIOs
#define PWM_BASE_BUS				0x7E20C000

// BCM Magic
#define	BCM_PASSWORD				0x5A000000



#define PWM_CLK_SRC_REQ				19200000ull
typedef enum GPIO_FUN_SELECT
{
	FSEL_INPUT	= 0b0000,
	FSEL_OUTPUT	= 0b0001,
	FSEL_ALT_0	= 0b0100,
	FSEL_ALT_1	= 0b0101,
	FSEL_ALT_2	= 0b0110,
	FSEL_ALT_3	= 0b0111,
	FSEL_ALT_4	= 0b0011,
	FSEL_ALT_5	= 0b0010,
}PinSel_t;

enum ONOFF
{
	OFF = 0,
	ON = 1,
};
#endif /* RPI3BCONSTANTS_H_ */
