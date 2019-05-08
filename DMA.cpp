#include <iostream>
#include <sys/mman.h> //for mmap
#include <unistd.h> //for NULL
#include <stdio.h> //for printf
#include <stdlib.h> //for exit
#include <fcntl.h> //for file opening
#include <stdint.h> //for uint32_t
#include <string.h> //for memset

#include "DMA.h" // for DMA addresses, etc.

uint32_t DMACtrl::channel_in_use = 0;
volatile DMAReg_t *DMACtrl::dma_regs = NULL;
int32_t DMACtrl::dma_instances = 0;

int dma_main()
{
	DMACtrl dma(5, 12);
	dma.dma_demo();
	return 0;
}

DMACtrl::DMACtrl(int32_t channel_num, uint32_t src_len)
	: m_ch(channel_num),
	  m_dest_virtual(NULL),
	  m_dest_physical(NULL)
{
	if (src_len > (uint32_t)getpagesize())
	{
		std::cout << "Doesn't support DMA more than PAGE_SIZE(" << getpagesize() << ") bytes of data" << std::endl;
		exit(1);
	}

	if (DMACtrl::GeneralInit(channel_num) >= 0)
	{
		AllocMemory(&m_src_virtual);
		GetPhyAddr(&m_src_virtual, &m_src_physical);

		AllocMemory(&m_cb_virtual);
		GetPhyAddr(&m_cb_virtual, &m_cb_physical);

		DMACtrl::MarkChannelInUse(channel_num);
	}
	++dma_instances;
}

DMACtrl::~DMACtrl()
{
	FreeMemory(&m_src_virtual);
	FreeMemory(&m_cb_virtual);

	//ToDo: How about peripheral destination?
	FreeMemory(&m_dest_virtual);

	channel_in_use &= ~(0x1 << m_ch);
	--dma_instances;
	if (0 == dma_instances)
	{
		munmap((void *)dma_regs, sizeof(DMAReg_t));
		dma_regs = NULL;
	}
}

volatile void *DMACtrl::getSrcVirtAddr()
{
	return m_src_virtual;
}

uint32_t DMACtrl::getSrcPhyAddr()
{
	return (uint32_t)m_src_physical;
}

volatile DMACtrlBlock_t *DMACtrl::getCBVirtAddr()
{
	return (volatile DMACtrlBlock_t *)m_cb_virtual;
}

uint32_t DMACtrl::getCBPhyAddr()
{
	return (uint32_t)m_cb_physical;
}

uint32_t DMACtrl::getDestPhyAddr()
{
	return (uint32_t)m_dest_physical;
}


volatile void *DMACtrl::SetDMADest(uint32_t dest_phy_addr, int32_t len)
{
	m_dest_physical = (volatile void *)dest_phy_addr;
	m_dest_virtual = (volatile void *)mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, dest_phy_addr);
	return m_dest_virtual;
}

int32_t DMACtrl::GeneralInit(int32_t channel_num)
{

	dma_regs = (volatile DMAReg_t *)mmap(NULL, sizeof(DMAReg_t), PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, DMA_BASE_ADDR);
	if (dma_regs == MAP_FAILED)
	{
		std::cout << "Failed to map dma_regs!" << std::endl;
		exit(2);
	}

	if (channel_in_use & (0x1 << channel_num))
	{
		std::cout << "Channel " << channel_num << " is already in use!\n";
		return -1;
	}

	return 0;
}

void DMACtrl::MarkChannelInUse(int32_t channel_num)
{
	DMACtrl::channel_in_use |= 0x1 << channel_num;
}

int32_t DMACtrl::AllocMemory(volatile void **vir)
{
	*vir = valloc(getpagesize()); //allocate one page of RAM

	//force page into RAM and then lock it there:
	((int*) *vir)[0] = 1;
	mlock((const void *)*vir, getpagesize());
	memset((void *)*vir, 0, getpagesize());

	return 0;
}

int32_t DMACtrl::GetPhyAddr(volatile void **vir, volatile void **phy)
{
	//Magic to determine the physical address for this page:
	uint64_t pageInfo;
	int file = open("/proc/self/pagemap", 'r');
	lseek(file, ((size_t) *vir) >> 9, SEEK_SET);
	read(file, &pageInfo, 8);

	*phy = (void*) (size_t) (pageInfo * getpagesize());
	printf("GetPhyAddr virtual to phys: %p -> %p\n", *vir, *phy);
	return 0;
}

void DMACtrl::FreeMemory(volatile void **vir)
{
	if (*vir != NULL)
	{
		munlock((const void *)vir, getpagesize());
		free((void *)*vir);
	}
}

volatile void *DMACtrl::AllocateDestMem()
{
	AllocMemory(&m_dest_virtual);
	GetPhyAddr(&m_dest_virtual, &m_dest_physical);
	return m_dest_virtual;
}

void DMACtrl::debugPrintDMARegs()
{
	printf("+m_ch: %d\n", m_ch);
	printf("sizeof(DMAChannel_t): %d\n", sizeof(DMAChannel_t));
	printf("+CtrlStatus: \t0x%08x\n", dma_regs->ch[m_ch].cs.word);
	printf("+CBAddr: \t0x%08x, CBAddr: 0x%08x\n", dma_regs->ch[m_ch].cbAddr, (uint32_t)m_cb_physical);
	printf("+TransInfo: \t0x%08x\n", dma_regs->ch[5].transInfo.word);
	printf("+srcAddr: \t0x%08x, SrcAddr: 0x%08x\n", dma_regs->ch[m_ch].srcAddr, (uint32_t)m_src_physical);
	printf("+destAddr: \t0x%08x, DestAddr: 0x%08x\n", dma_regs->ch[m_ch].destAddr, (uint32_t)m_dest_physical);
	printf("+transLen: \t0x%08x\n", dma_regs->ch[m_ch].translen.word);
	printf("+Stride: \t0x%08x\n", dma_regs->ch[m_ch].stride.word);
	printf("+nextCB: \t0x%08x\n", dma_regs->ch[m_ch].nextCB);
	printf("+Debug: \t0x%08x\n", dma_regs->ch[m_ch].debug.word);
}

void DMACtrl::dma_demo()
{
	std::cout << "\n\ndma_test start" << std::endl;

	AllocateDestMem();

	char *srcArray = (char*) m_src_virtual;
	srcArray[0] = 'h';
	srcArray[1] = 'e';
	srcArray[2] = 'l';
	srcArray[3] = 'l';
	srcArray[4] = 'o';
	srcArray[5] = ' ';
	srcArray[6] = 'w';
	srcArray[7] = 'o';
	srcArray[8] = 'r';
	srcArray[9] = 'l';
	srcArray[10] = 'd';
	srcArray[11] = '\0'; //null terminator used for printf call.

	volatile DMACtrlBlock_t *cb = (volatile DMACtrlBlock_t *)m_cb_virtual;
	cb->transInfo.word = 0;
	cb->transInfo.src_inc = 1;
	cb->transInfo.dest_inc = 1;
	cb->srcAddr = (uint32_t) m_src_physical; //set source and destination DMA address
	cb->destAddr = (uint32_t) m_dest_physical;
	cb->transLen.x_len = 12; //transfer 12 bytes
	cb->stride.word = 0; //no 2D stride
	cb->nextCB = DMACtrl::NO_NEXT_CB; //no next control block

	dma_regs->enable |= 0x1 << m_ch;
	dma_regs->ch[m_ch].cs.reset = 1;
	sleep(1);
	dma_regs->ch[m_ch].debug.read_err = 1;
	dma_regs->ch[m_ch].debug.fifo_err = 1;
	dma_regs->ch[m_ch].debug.read_last_not_set_err = 1;
	dma_regs->ch[m_ch].cbAddr = (uint32_t) m_cb_physical;
	dma_regs->ch[m_ch].cs.active = 1;
	//DMACtrl::debugPrintDMARegs();
	sleep(1);
	std::cout << "src: " << (char *)m_src_virtual << std::endl;
	std::cout << "dest: " << (char *)m_dest_virtual << std::endl;
}
