/*
 * MemBase.cpp
 *
 *  Created on: May 7, 2019
 *      Author: aguan
 */
#include <iostream>
#include <iomanip>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <bitset>
#include <string>
#include "MemBase.h"

int32_t MemBase::mem_fd = -1;
int32_t MemBase::num_of_mem_inst = 0;

MemBase::MemBase()
{
	if (0 == num_of_mem_inst)
		Init();
	++num_of_mem_inst;
}

MemBase::~MemBase()
{
	--num_of_mem_inst;
	if (0 == num_of_mem_inst)
		Uninit();
}

int32_t MemBase::Init()
{
	//Check whether the "/dev/mem" has been opened or not
	if (mem_fd != 0)
	{
		try
		{
			mem_fd = open("/dev/mem", O_RDWR | O_SYNC | O_CLOEXEC);
			if (mem_fd < 0)
				throw "Failed to open /dev/mem";
		}
		catch (const std::string &err)
		{
			std::cout << __func__ << " with exception: " << err << std::endl;
			return -1;
		}
		catch (...)
		{
			std::cout << __func__ << " with unknown exception" << std::endl;
			return -2;
		}
	}
	return 0;
}

void MemBase::Uninit()
{
	close(mem_fd);
	mem_fd = -1;
}
