/*
 * MemBase.h
 *
 *  Created on: May 7, 2019
 *      Author: Albert Guan
 */
#include <stdint.h>
#include <stddef.h>     /* offsetof */
#include <vector>
#pragma once

class MemBase
{
public:
	MemBase();
	virtual ~MemBase();
protected:
	static int32_t Init();
	static void Uninit();
	static int32_t mem_fd;
private:
	static int32_t num_of_mem_inst;
};
