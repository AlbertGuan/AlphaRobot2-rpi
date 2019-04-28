#include <iostream>
#include <sys/mman.h> //for mmap
#include <unistd.h> //for NULL
#include <stdio.h> //for printf
#include <stdlib.h> //for exit
#include <fcntl.h> //for file opening
#include <stdint.h> //for uint32_t
#include <string.h> //for memset

#include "DMA.h" // for DMA addresses, etc.

#define PAGE_SIZE 4096 //mmap maps pages of memory, so we must give it multiples of this size

//-------- Relative offsets for DMA registers
//DMA Channel register sets (format of these registers is found in DmaChannelHeader struct):
#define DMACH(n) (0x100*(n))
//Each DMA channel has some associated registers, but only CS (control and status), CONBLK_AD (control block address), and DEBUG are writeable
//DMA is started by writing address of the first Control Block to the DMA channel's CONBLK_AD register and then setting the ACTIVE bit inside the CS register (bit 0)
//Note: DMA channels are connected directly to peripherals, so physical addresses should be used (affects control block's SOURCE, DEST and NEXTCONBK addresses).
#define DMAENABLE 0x00000ff0 //bit 0 should be set to 1 to enable channel 0. bit 1 enables channel 1, etc.

//flags used in the DmaChannelHeader struct:
#define DMA_CS_RESET (1<<31)
#define DMA_CS_ACTIVE (1<<0)

#define DMA_DEBUG_READ_ERROR (1<<2)
#define DMA_DEBUG_FIFO_ERROR (1<<1)
#define DMA_DEBUG_READ_LAST_NOT_SET_ERROR (1<<0)

//flags used in the DmaControlBlock struct:
#define DMA_CB_TI_DEST_INC (1<<4)
#define DMA_CB_TI_SRC_INC (1<<8)


struct DmaChannelHeader
{
	uint32_t CS; //Control and Status
	//31    RESET; set to 1 to reset DMA
	//30    ABORT; set to 1 to abort current DMA control block (next one will be loaded & continue)
	//29    DISDEBUG; set to 1 and DMA won't be paused when debug signal is sent
	//28    WAIT_FOR_OUTSTANDING_WRITES; set to 1 and DMA will wait until peripheral says all writes have gone through before loading next CB
	//24-27 reserved
	//20-23 PANIC_PRIORITY; 0 is lowest priority
	//16-19 PRIORITY; bus scheduling priority. 0 is lowest
	//9-15  reserved
	//8     ERROR; read as 1 when error is encountered. error can be found in DEBUG register.
	//7     reserved
	//6     WAITING_FOR_OUTSTANDING_WRITES; read as 1 when waiting for outstanding writes
	//5     DREQ_STOPS_DMA; read as 1 if DREQ is currently preventing DMA
	//4     PAUSED; read as 1 if DMA is paused
	//3     DREQ; copy of the data request signal from the peripheral, if DREQ is enabled. reads as 1 if data is being requested, else 0
	//2     INT; set when current CB ends and its INTEN=1. Write a 1 to this register to clear it
	//1     END; set when the transfer defined by current CB is complete. Write 1 to clear.
	//0     ACTIVE; write 1 to activate DMA (load the CB before hand)
	uint32_t CONBLK_AD; //Control Block Address
	uint32_t TI; //transfer information; see DmaControlBlock.TI for description
	uint32_t SOURCE_AD; //Source address
	uint32_t DEST_AD; //Destination address
	uint32_t TXFR_LEN; //transfer length.
	uint32_t STRIDE; //2D Mode Stride. Only used if TI.TDMODE = 1
	uint32_t NEXTCONBK; //Next control block. Must be 256-bit aligned (32 bytes; 8 words)
	uint32_t DEBUG; //controls debug settings
};

struct DmaControlBlock
{
	uint32_t TI; //transfer information
	//31:27 unused
	//26    NO_WIDE_BURSTS
	//21:25 WAITS; number of cycles to wait between each DMA read/write operation
	//16:20 PERMAP; peripheral number to be used for DREQ signal (pacing). set to 0 for unpaced DMA.
	//12:15 BURST_LENGTH
	//11    SRC_IGNORE; set to 1 to not perform reads. Used to manually fill caches
	//10    SRC_DREQ; set to 1 to have the DREQ from PERMAP gate requests.
	//9     SRC_WIDTH; set to 1 for 128-bit moves, 0 for 32-bit moves
	//8     SRC_INC;   set to 1 to automatically increment the source address after each read (you'll want this if you're copying a range of memory)
	//7     DEST_IGNORE; set to 1 to not perform writes.
	//6     DEST_DREG; set to 1 to have the DREQ from PERMAP gate *writes*
	//5     DEST_WIDTH; set to 1 for 128-bit moves, 0 for 32-bit moves
	//4     DEST_INC;   set to 1 to automatically increment the destination address after each read (Tyou'll want this if you're copying a range of memory)
	//3     WAIT_RESP; make DMA wait for a response from the peripheral during each write. Ensures multiple writes don't get stacked in the pipeline
	//2     unused (0)
	//1     TDMODE; set to 1 to enable 2D mode
	//0     INTEN;  set to 1 to generate an interrupt upon completion
	uint32_t SOURCE_AD; //Source address
	uint32_t DEST_AD; //Destination address
	uint32_t TXFR_LEN; //transfer length.
	uint32_t STRIDE; //2D Mode Stride. Only used if TI.TDMODE = 1
	uint32_t NEXTCONBK; //Next control block. Must be 256-bit aligned (32 bytes; 8 words)
	uint32_t _reserved[2];
};

//set bits designated by (mask) at the address (dest) to (value), without affecting the other bits
//eg if x = 0b11001100
//  writeBitmasked(&x, 0b00000110, 0b11110011),
//  then x now = 0b11001110
void writeBitmasked(volatile uint32_t *dest, uint32_t mask, uint32_t value)
{
	uint32_t cur = *dest;
	uint32_t another = (cur & (~mask)) | (value & mask);
	*dest = another;
	*dest = another; //added safety for when crossing memory barriers.
}

//allocate a page & simultaneously determine its physical address.
//virtAddr and physAddr are essentially passed by-reference.
//this allows for:
//void *virt, *phys;
//makeVirtPhysPage(&virt, &phys)
//now, virt[N] exists for 0 <= N < PAGE_SIZE,
//  and phys+N is the physical address for virt[N]
//based on http://www.raspians.com/turning-the-raspberry-pi-into-an-fm-transmitter/
void makeVirtPhysPage(volatile void** virtAddr, volatile void** physAddr)
{
	*virtAddr = valloc(PAGE_SIZE); //allocate one page of RAM

	//force page into RAM and then lock it there:
	((int*) *virtAddr)[0] = 1;
	mlock((const void *)*virtAddr, PAGE_SIZE);
	memset((void *)*virtAddr, 0, PAGE_SIZE); //zero-fill the page for convenience

	//Magic to determine the physical address for this page:
	uint64_t pageInfo;
	int file = open("/proc/self/pagemap", 'r');
	lseek(file, ((size_t) *virtAddr) >> 9, SEEK_SET);
	read(file, &pageInfo, 8);

	*physAddr = (void*) (size_t) (pageInfo * PAGE_SIZE);
	printf("makeVirtPhysPage virtual to phys: %p -> %p\n", *virtAddr, *physAddr);
}

//call with virtual address to deallocate a page allocated with makeVirtPhysPage
void freeVirtPhysPage(volatile void* virtAddr)
{
	munlock((const void *)virtAddr, PAGE_SIZE);
	free((void *)virtAddr);
}

//map a physical address into our virtual address space. memfd is the file descriptor for /dev/mem
volatile uint32_t* mapPeripheral(int memfd, int addr)
{
	///dev/mem behaves as a file. We need to map that file into memory:
	void *mapped = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, memfd, addr);
	//now, *mapped = memory at physical address of addr.
	if (mapped == MAP_FAILED)
	{
		printf("failed to map memory (did you remember to run as root?)\n");
		exit(1);
	}
	else
	{
		printf("mapped: %p\n", mapped);
	}
	return (volatile uint32_t*) mapped;
}

int dma_main()
{
	//cat /sys/module/dma/parameters/dmachans gives a bitmask of DMA channels that are not used by GPU. Results: ch 1, 3, 6, 7 are reserved.
	//dmesg | grep "DMA"; results: Ch 2 is used by SDHC host
	//ch 0 is known to be used for graphics acceleration
	//Thus, applications can use ch 4, 5, or the LITE channels @ 8 and beyond.
	int dmaChNum = 5;
	//First, open the linux device, /dev/mem
	//dev/mem provides access to the physical memory of the entire processor+ram
	//This is needed because Linux uses virtual memory, thus the process's memory at 0x00000000 will NOT have the same contents as the physical memory at 0x00000000
	int memfd = open("/dev/mem", O_RDWR | O_SYNC);
	if (memfd < 0)
	{
		printf("Failed to open /dev/mem (did you remember to run as root?)\n");
		exit(1);
	}
	//now map /dev/mem into memory, but only map specific peripheral sections:
	volatile uint32_t *dmaBaseMem = mapPeripheral(memfd, DMA_BASE);

	//configure DMA:
	//allocate 1 page for the source and 1 page for the destination:
	volatile void *virtSrcPage, *physSrcPage;
	makeVirtPhysPage(&virtSrcPage, &physSrcPage);
	volatile void *virtDestPage, *physDestPage;
	makeVirtPhysPage(&virtDestPage, &physDestPage);

	//write a few bytes to the source page:
	char *srcArray = (char*) virtSrcPage;
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

	//allocate 1 page for the control blocks
	volatile void *virtCbPage, *physCbPage;
	makeVirtPhysPage(&virtCbPage, &physCbPage);

	//dedicate the first 8 words of this page to holding the cb.
	volatile struct DmaControlBlock *cb1 = (struct DmaControlBlock*) virtCbPage;


	//fill the control block:
	cb1->TI = DMA_CB_TI_SRC_INC | DMA_CB_TI_DEST_INC; //after each byte copied, we want to increment the source and destination address of the copy, otherwise we'll be copying to the same address.
	cb1->SOURCE_AD = (uint32_t) physSrcPage; //set source and destination DMA address
	cb1->DEST_AD = (uint32_t) physDestPage;
	cb1->TXFR_LEN = 12; //transfer 12 bytes
	cb1->STRIDE = 0; //no 2D stride
	cb1->NEXTCONBK = 0; //no next control block

	printf("destination was initially: src: %s (%p), dest: %s, cb addr: 0x%p\n", (char *)virtSrcPage, physSrcPage, (char*) virtDestPage, physCbPage);

	//enable DMA channel (it's probably already enabled, but we want to be sure):
	writeBitmasked(dmaBaseMem + DMAENABLE / 4, 1 << dmaChNum, 1 << dmaChNum);

	//configure the DMA header to point to our control block:
	volatile struct DmaChannelHeader *dmaHeader = (volatile struct DmaChannelHeader*) (dmaBaseMem + (DMACH(dmaChNum)) / 4); //dmaBaseMem is a uint32_t ptr, so divide by 4 before adding byte offset
	dmaHeader->CS = DMA_CS_RESET; //make sure to disable dma first.
	sleep(1); //give time for the reset command to be handled.
	dmaHeader->DEBUG = DMA_DEBUG_READ_ERROR | DMA_DEBUG_FIFO_ERROR | DMA_DEBUG_READ_LAST_NOT_SET_ERROR; // clear debug error flags
	dmaHeader->CONBLK_AD = (uint32_t) physCbPage; //we have to point it to the PHYSICAL address of the control block (cb1)
	dmaHeader->CS = DMA_CS_ACTIVE; //set active bit, but everything else is 0.

	sleep(1); //give time for copy to happen

	printf("destination reads: src: %s, dest: %s\n", (char *)virtSrcPage, (char*) virtDestPage);

	printf("dmaHeader->CS: \t0x%08x\n", dmaHeader->CS);
	printf("dmaHeader->CONBLK_AD: \t0x%08x\t0x%08x\n", dmaHeader->CONBLK_AD, (uint32_t)physCbPage);
	printf("dmaHeader->TI: \t0x%08x\n", dmaHeader->TI);
	printf("dmaHeader->SOURCE_AD: \t0x%08x\t0x%08x\n", dmaHeader->SOURCE_AD, (uint32_t)physSrcPage);
	printf("dmaHeader->DEST_AD: \t0x%08x\t0x%08x\n", dmaHeader->DEST_AD, (uint32_t)physDestPage);
	printf("dmaHeader->TXFR_LEN: \t0x%08x\n", dmaHeader->TXFR_LEN);
	printf("dmaHeader->STRIDE: \t0x%08x\n", dmaHeader->STRIDE);
	printf("dmaHeader->NEXTCONBK: \t0x%08x\n", dmaHeader->NEXTCONBK);
	printf("dmaHeader->DEBUG: \t0x%08x\n", dmaHeader->DEBUG);

	dmaHeader->CS = DMA_CS_RESET;
	sleep(1);

	//cleanup
	freeVirtPhysPage(virtCbPage);
	freeVirtPhysPage(virtDestPage);
	freeVirtPhysPage(virtSrcPage);

	DMACtrl dma(5, 12);
	dma.dma_test();
	return 0;
}

int32_t DMACtrl::mem_fd = -1;
uint32_t DMACtrl::channel_in_use = 0;
volatile DMAReg_t *DMACtrl::dma_regs = NULL;

DMACtrl::DMACtrl(int32_t channel_num, uint32_t src_len)
	: m_ch(channel_num)
{
	if (src_len > PAGE_SIZE)
	{
		std::cout << "Doesn't support DMA more than PAGE_SIZE(" <<PAGE_SIZE << ") bytes of data" << std::endl;
		exit(1);
	}

	if (DMACtrl::GeneralInit(channel_num) >= 0)
	{
		AllocMemory(&m_src_virtual);
		GetPhyAddr(&m_src_virtual, &m_src_physical);

		AllocMemory(&m_dest_virtual);
		GetPhyAddr(&m_dest_virtual, &m_dest_physical);

		AllocMemory(&m_cb_virtual);
		GetPhyAddr(&m_cb_virtual, &m_cb_physical);

		DMACtrl::MarkChannelInUse(channel_num);
	}
}

DMACtrl::~DMACtrl()
{

}

volatile void *DMACtrl::getSrcVirtAddr()
{
	return m_src_virtual;
}

void DMACtrl::dma_test()
{
	std::cout << "\n\ndma_test start" << std::endl;
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
	DMACtrl::debugPrintDMARegs();
	sleep(1);
	std::cout << "src: " << (char *)m_src_virtual << std::endl;
	std::cout << "dest: " << (char *)m_dest_virtual << std::endl;
}

int32_t DMACtrl::GeneralInit(int32_t channel_num)
{
	if (mem_fd < 0)
	{
		mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
		if (mem_fd < 0)
		{
			std::cout << "Failed to open /dev/mem (did you remember to run as root?)\n";
			exit(1);
		}

		dma_regs = (volatile DMAReg_t *)mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, DMA_BASE);
		if (dma_regs == MAP_FAILED)
		{
			std::cout << "Failed to map dma_regs!" << std::endl;
			exit(2);
		}
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
	*vir = valloc(PAGE_SIZE); //allocate one page of RAM

	//force page into RAM and then lock it there:
	((int*) *vir)[0] = 1;
	mlock((const void *)*vir, PAGE_SIZE);
	memset((void *)*vir, 0, PAGE_SIZE);

	return 0;
}

int32_t DMACtrl::GetPhyAddr(volatile void **vir, volatile void **phy)
{
	//Magic to determine the physical address for this page:
	uint64_t pageInfo;
	int file = open("/proc/self/pagemap", 'r');
	lseek(file, ((size_t) *vir) >> 9, SEEK_SET);
	read(file, &pageInfo, 8);

	*phy = (void*) (size_t) (pageInfo * PAGE_SIZE);
	printf("GetPhyAddr virtual to phys: %p -> %p\n", *vir, *phy);
	return 0;
}

void DMACtrl::FreeMemory(volatile void **vir)
{
	munlock((const void *)vir, PAGE_SIZE);
	free((void *)vir);
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
