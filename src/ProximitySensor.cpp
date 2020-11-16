/*
 * ProximitySensor.cpp
 *
 *  Created on: May 29, 2019
 *      Author: aobog
 */

#include <iostream>
#include <vector>
#include <iomanip>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <bitset>
#include <assert.h>
#include <exception>
#include "ProximitySensor.h"

void ProximitySensorTest()
{
	GpioIn left(16);
	GpioIn right(19);

	std::vector<int32_t> rise_pins;
	while (1)
	{
		//GpioIn::CheckInput(std::vector<int32_t>{left[0], right[0]}, rise_pins, GpioIn::High);
		std::cout << "Rising pins: " << rise_pins.size() << std::endl;
		rise_pins.clear();

		sleep(1);
	}
}
