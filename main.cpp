/*
 * main.cpp
 *
 *  Created on: Mar 30, 2019
 *      Author: aobog
 */
#include <iostream>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "bcm2835.h"

#include <wiringPi.h>

#include "CameraMotors.h"
#include "RGBControl.h"

void gpio_17_18_led_ctrl()
{
	int fd;
	if ((fd = open("/dev/mem", ( O_RDWR | O_SYNC))) == -1)
	{
		printf("ERROR: could not open \"/dev/mem\"...\n");
		return;
	}

	//Map the address
	void *gpio_base = (unsigned int *)mmap(NULL, BCM2835_PERI_SIZE, ( PROT_READ | PROT_WRITE), MAP_SHARED, fd, BCM2835_PERI_BASE);

	void *gpio_sel_17 = (void *)((unsigned int)gpio_base + GPIO_SEL1_OFFSET);
	void *gpio_sel_18 = (void *)((unsigned int)gpio_base + GPIO_SEL1_OFFSET);
	//Set as output
	*(unsigned int *)gpio_sel_17 &= ~0x00E00000;
	*(unsigned int *)gpio_sel_17 |= 0x1 << 21;
	*(unsigned int *)gpio_sel_18 &= ~0x07000000;
	*(unsigned int *)gpio_sel_18 |= 0x1 << 24;

	//On and off
	while (1)
	{
		*(unsigned int *)((unsigned int)gpio_base + GPIO_SET0_OFFSET) |= 0x3 << 17;
		usleep(200000);
		*(unsigned int *)((unsigned int)gpio_base + GPIO_CLR0_OFFSET) |= 0x3 << 17;
		usleep(200000);
	}
}

int main(int argc, char *argv[])
{
	using namespace std;
	cout << "Hello Raspberry Pi!" << endl;

	//rpiI2CInit();

	RBGControl();
	return 0;
}


