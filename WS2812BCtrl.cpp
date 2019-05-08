/*
 * WS2812BCtrl.cpp
 *
 *  Created on: Apr 14, 2019
 *      Author: aobog
 */
#include "WS2812BCtrl.h"

#include <iostream>
#include <iomanip>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>


#define WS2812B_WIDTH		40
#define WS2812B_LOGIC_1		WS2812B_WIDTH * 60 /100
#define WS2812B_LOGIC_0		WS2812B_WIDTH - WS2812B_LOGIC_1
#define WS2812B_END			0


WS2812BCtrl::WS2812BCtrl(float brightness)
{
	m_pwm = new PWMCtrl(18, WS2812B_PWM_RANGE, WS2812B_PWM_DIVIDOR);
	if (brightness > 1.0)
		m_brightness = 1;
	else if (brightness < 0.0)
		m_brightness = 0.0;
	else
		m_brightness = brightness;
}

WS2812BCtrl::~WS2812BCtrl()
{

}

typedef struct
{
	uint32_t R;
	uint32_t G;
	uint32_t B;
}RGB_t;

void WS2812BCtrl::Init()
{
	//Start Init PWM
	pwm_reg_CTL_t pwm_ctl;
	pwm_ctl.word = m_pwm->GetPWMCTL().word & 0xFFFFFF00;

	//Enable the PWM channel 1
	pwm_ctl.PWEN1 = 0;
	//Set to serialiser mode
	pwm_ctl.MODE1 = 1;
	//Use the FIFO
	pwm_ctl.USEF1 = 1;
	//Clear the FIFO
	pwm_ctl.CLRF1 = 1;

	m_pwm->SetPWMCtrl(pwm_ctl);
	usleep(200);
}

//Each led requires 24 pixels, each pixel contains 3 bits in "arr"(0b110 for "1" and 0b100 for "0")
void WS2812BCtrl::setSerializedRGB(uint32_t *arr, const int led_idx, const LEDPixel_t &color)
{
	uint32_t R = color.R * m_brightness;
	uint32_t G = color.G * m_brightness;
	uint32_t B = color.B * m_brightness;
	uint32_t color_comp = (G << 16) | (R << 8) | B;
//	printf("led[%d]: color_comp: 0x%08x\n", led_idx, color_comp);
	uint32_t mask = 0x1;
	for (int pixel = 0; pixel < 24; ++pixel, mask <<= 1)
	{
		int32_t word_off = (led_idx * 72 + pixel * 3) / 32;
		int32_t bit_off = 29 - (led_idx * 72 + pixel * 3) % 32;
		if (bit_off >= 0)
		{
			arr[word_off] &= ~(0x7 << bit_off);
			if (color_comp & mask)
				arr[word_off] |= 0x6 << bit_off;
			else
				arr[word_off] |= 0x4 << bit_off;
		}
		else if (bit_off == -1)
		{
			arr[word_off] &= ~(0x3);
			arr[word_off + 1] &= ~(0x1 << (32 + bit_off));
			if (color_comp & mask)
				arr[word_off] |= 0x3;
			else
				arr[word_off] |= 0x2;
		}
		else if (bit_off == -2)
		{
			arr[word_off] |= 0x1;
			arr[word_off + 1] &= ~(0x3 << (32 + bit_off));
			if (color_comp & mask)
				arr[word_off + 1] |= 0x2 << (32 + bit_off);
		}
	}
}

void WS2812BCtrl::WaterLight()
{
	const uint32_t led_val = 255 * m_brightness;
	uint32_t vals[10];
	LEDPixel_t leds[4] = {
			{led_val, 0 ,0},
			{0, led_val, 0},
			{0, 0, led_val},
			{led_val, led_val, led_val}
	};

	for (int i = 0; i < 10; ++i)
		vals[i] = 0;
	setSerializedRGB(vals, 0, { 0, 0, 0 });
	setSerializedRGB(vals, 1, { 0, 0, 0 });
	setSerializedRGB(vals, 2, { 0, 0, 0 });
	setSerializedRGB(vals, 3, { 0, 0, 0 });

	int idx = 0;
	while (1)
	{
		++idx;
		setSerializedRGB(vals, 0, leds[idx % 4]);
		setSerializedRGB(vals, 1, leds[(idx + 1) % 4]);
		setSerializedRGB(vals, 2, leds[(idx + 2) % 4]);
		setSerializedRGB(vals, 3, leds[(idx + 3) % 4]);
		m_pwm->pwmWriteFIFO(vals, 9);
		pwm_reg_CTL_t pwm_ctl;
		pwm_ctl.word = m_pwm->GetPWMCTL().word;
		pwm_ctl.PWEN1 = 1;
		m_pwm->SetPWMCtrl(pwm_ctl);
		usleep(1000);

		pwm_ctl.word = m_pwm->GetPWMCTL().word;
		pwm_ctl.PWEN1 = 0;
		pwm_ctl.CLRF1 = 1;
		m_pwm->SetPWMCtrl(pwm_ctl);
		sleep(1);
	}
}
