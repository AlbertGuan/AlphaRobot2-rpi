/*
 * utilities.cpp
 *
 *  Created on: Apr 23, 2019
 *      Author: Albert Guan
 */
#include <iostream>

int32_t set_bits(uint32_t *org, uint32_t val, int lbit, int rbit)
{
	if (lbit < rbit)
	{
		std::cout << "Wrong lbit: " << lbit << " rbit: " << rbit << std::endl;
		return -1;
	}
	uint32_t max_val = 0x1u << (lbit - rbit + 1);
	if (max_val <= val)
	{
		std::cout << "val (" << val << ") is greater than max: " << max_val << std::endl;
		return -1;
	}
	//Get the mask
	uint32_t mask = (max_val - 1) << rbit;
	//clear target bits
	*org &= ~mask;
	//set target bits
	*org |= val << rbit;
	return 0;
}

