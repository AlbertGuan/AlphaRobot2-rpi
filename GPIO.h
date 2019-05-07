/*
 * GPIO.h
 *
 *  Created on: May 6, 2019
 *      Author: aobog
 */

#pragma once

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