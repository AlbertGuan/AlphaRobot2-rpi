/*
 * DMA.h
 *
 *  Created on: Apr 15, 2019
 *      Author: aobog
 */

#pragma once
#include "Rpi3BConstants.h"
#include "WS2812BCtrl.h"
/*
 * processor documentation for RPI1 at: http://www.raspberrypi.org/wp-content/uploads/2012/02/BCM2835-ARM-Peripherals.pdf
 * pg 38 for DMA
 */

int dma_main();

typedef union
{
	struct
	{
		uint32_t active					: 1;
		uint32_t end					: 1;
		uint32_t int_status				: 1;		//Set when transfer for the CB ends and INTEN is set to 1
		uint32_t dreq				: 1;		//Status of the selected data request signal
		uint32_t paused			: 1;
		uint32_t dreq_stops_dma	: 1;//Indicates the DMA is currently paused and not transferring data due to the DREQ being inactive
		uint32_t wait_for_write	: 1;
		uint32_t _reserve0		: 1;
		uint32_t error			: 1;
		uint32_t _reserve1		: 7;
		uint32_t priority				: 4;
		uint32_t panic_priority			: 4;
		uint32_t _reserve2		: 4;
		uint32_t wait_for_outstanding_wt: 1;
		uint32_t disable_debug			: 1;
		uint32_t abort					: 1;
		uint32_t reset					: 1;
	};
	uint32_t word;
}DMACtrlStaus_t;

typedef union
{
	struct
	{
		uint32_t int_en					: 1;
		uint32_t td_mode				: 1;		//2D mode
		uint32_t _reserve0				: 1;
		uint32_t wait_resp				: 1;
		uint32_t dest_inc				: 1;		//Destination address increments after each write
		uint32_t dest_width				: 1;		//1: use 128-bit destination write width, 0: use 32-bit
		uint32_t dest_dreq				: 1;
		uint32_t dest_ignore			: 1;
		uint32_t src_inc				: 1;		//Source address increment
		uint32_t src_width				: 1;
		uint32_t src_dreq				: 1;
		uint32_t src_ignore				: 1;
		uint32_t burst_len				: 4;
		uint32_t premap					: 5;		//Indicates the peripheral number whose ready signal shall be used to control the rate of transfers
		uint32_t wait_cycles			: 5;
		uint32_t no_wide_bursts			: 1;
		uint32_t _reserve1				: 5;
	};
	uint32_t word;
}DMATransInfo_t;

typedef union
{
	struct
	{
		uint32_t x_len : 16;
		uint32_t y_len : 16;
	};
	uint32_t word;
}DMATransLen_t;

typedef union
{
	struct
	{
		uint32_t s_stride : 16;
		uint32_t d_stride : 16;
	};
	uint32_t word;
}DMAStride_t;

typedef union
{
	struct
	{
		uint32_t read_last_not_set_err		: 1;
		uint32_t fifo_err					: 1;
		uint32_t read_err					: 1;
		uint32_t _reserve0					: 1;
		const uint32_t outstanding_writes	: 1;
		const uint32_t dma_id				: 8;
		const uint32_t dma_state			: 9;
		const uint32_t dma_version			: 3;
		const uint32_t lite					: 1;
		const uint32_t _reserve1			: 3;
	};
	uint32_t word;
}DMADebug_t;

typedef struct
{
	DMATransInfo_t transInfo;			//TI
	uint32_t srcAddr;					//SOURCE_AD
	uint32_t destAddr;					//DEST_AD
	DMATransLen_t transLen;				//TXFR_LEN
	DMAStride_t stride;					//STRIDE
	uint32_t nextCB;					//NEXTCONBK
	uint32_t _reserve[2];
}DMACtrlBlock_t;


typedef struct
{
	DMACtrlStaus_t cs;				//Control and Status
	uint32_t cbAddr;			//Control block address
	DMATransInfo_t transInfo;
	uint32_t srcAddr;
	uint32_t destAddr;
	DMATransLen_t translen;
	DMAStride_t stride;
	uint32_t nextCB;
	DMADebug_t debug;
	const uint32_t _padding[0xDC >> 2];
}DMAChannel_t;

typedef struct
{
	DMAChannel_t ch[15];
	const uint32_t _padding[0x1BC >> 2];
	uint32_t int_status;
	const uint32_t _padding1[0xC >> 2];
	uint32_t enable;
}DMAReg_t;

class DMACtrl
{
public:
	DMACtrl(int32_t channel_num, uint32_t src_len);
	~DMACtrl();

	static int32_t GeneralInit(int32_t channel_num);
	static void MarkChannelInUse(int32_t channel_num);
	static int32_t AllocMemory(volatile void **vir);
	static void FreeMemory(volatile void **vir);
	static int32_t GetPhyAddr(volatile void **vir, volatile void **phy);

	void debugPrintDMARegs();

	volatile void *getSrcVirtAddr();
	uint32_t getSrcPhyAddr();
	volatile DMACtrlBlock_t *getCBVirtAddr();
	uint32_t getCBPhyAddr();
	volatile void *AllocateDestMem();
	volatile void *SetDMADest(uint32_t dest_phy_addr, int32_t len);
	uint32_t getDestPhyAddr();

	void dma_demo();

	friend class WS2812BCtrl;

	enum
	{
		NO_USE	= 0,
		DSI		= 1,
		PCM_TX	= 2,
		PCM_RX	= 3,
		SMI		= 4,
		PWM		= 5
	};
private:
	static const uint32_t DMA_BASE_ADDR = PERIPHERAL_PHY_BASE + DMA_OFFSET;
	static const uint32_t NO_NEXT_CB = 0x00000000;	//When nextCB is set to it, DMA controller won't load further CBs, and stop the DMA after current transfer
	static int32_t mem_fd;
	static uint32_t channel_in_use;
	static volatile DMAReg_t *dma_regs;
	static int32_t dma_instances;

	int32_t m_ch;
	volatile void *m_src_virtual;
	volatile void *m_src_physical;
	volatile void *m_dest_virtual;
	volatile void *m_dest_physical;
	volatile void *m_cb_virtual;
	volatile void *m_cb_physical;
};
