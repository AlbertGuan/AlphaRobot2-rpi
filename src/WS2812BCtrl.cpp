/*
 * WS2812BCtrl.cpp
 *
 *  Created on: Apr 14, 2019
 *      Author: Albert Guan
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
#include <assert.h>

WS2812BCtrl::WS2812BCtrl (
	_In_ float brightness
	)

/*
 Routine Description:

	This routine is the constructor of WS2812BCtrl. It inits the PWM control
	module and sets LED brightness.

 Parameters:

 	Brightness - Supplies the brightness of LEDs.

 Return Value:

	None.

*/

{

	m_PWM = new GpioPwm(WS2812B_PIN,
						WS2812B_PWM_RANGE,
						WS2812B_PWM_DIVIDOR,
						WS2812B_PWM_MODE,
						WS2812B_PWM_FIFO);

	//
	// Init the brightness
	//

	SetBrightness(brightness);
	return;
}

WS2812BCtrl::~WS2812BCtrl (
	void
	)

/*
 Routine Description:

	This routine is the destructor of WS2812BCtrl.

 Parameters:

 	None.

 Return Value:

	None.

*/

{

	if (m_PWM != NULL) {
		delete m_PWM;
	}

	m_PWM = NULL;
	return;
}

void
WS2812BCtrl::SetBrightness (
	_In_ float Brightness
	)

/*
 Routine Description:

	This routine updates the LED brightness.

 Parameters:

 	Brightness - Supplies the brightness value.

 Return Value:

	None.

*/

{
	assert((Brightness < 1.0) && (Brightness > 0.0));

	if ((Brightness <= 1.0) && (Brightness >= 0.0)) {
		m_Brightness = Brightness;
	}

	return;
}

void
WS2812BCtrl::setSerializedRGB (
	_Out_ uint32_t *arr,
	_In_ const int led_idx,
	_In_ const LEDPixel &color
	)

/*
 Routine Description:

	This routine updates LED pixels based on user's input. Each led requires 24
	pixels, each pixel contains 3 bits in "arr"(0b110 for "1" and 0b100 for "0")

 Parameters:

 	Brightness - Supplies the brightness value.

 Return Value:

	None.

*/

{

	uint32_t R = color.R * m_Brightness;
	uint32_t G = color.G * m_Brightness;
	uint32_t B = color.B * m_Brightness;
	uint32_t color_comp = (G << 16) | (R << 8) | B;
	uint32_t mask = 0x1;

	for (int pixel = 0; pixel < 24; ++pixel, mask <<= 1) {
		int32_t word_off = (led_idx * 72 + pixel * 3) / 32;
		int32_t bit_off = 29 - (led_idx * 72 + pixel * 3) % 32;
		if (bit_off >= 0) {
			arr[word_off] &= ~(0x7 << bit_off);
			if (color_comp & mask) {
				arr[word_off] |= 0x6 << bit_off;

			} else {
				arr[word_off] |= 0x4 << bit_off;
			}

		} else if (bit_off == -1) {
			arr[word_off] &= ~(0x3);
			arr[word_off + 1] &= ~(0x1 << (32 + bit_off));
			if (color_comp & mask) {
				arr[word_off] |= 0x3;

			} else {
				arr[word_off] |= 0x2;
			}

		} else if (bit_off == -2) {
			arr[word_off] |= 0x1;
			arr[word_off + 1] &= ~(0x3 << (32 + bit_off));
			if (color_comp & mask) {
				arr[word_off + 1] |= 0x2 << (32 + bit_off);
			}
		}
	}
}

void
WS2812BCtrl::OneShot (
	_In_ uint32_t *vals,
	_In_ uint32_t len
	)

{
	m_PWM->UpdatePWMFIFO(vals, 9);
			m_PWM->PWMOnOff(ON);
			usleep(1000);

	m_PWM->PWMOnOff(OFF);
	m_PWM->ClearFIFO();
	sleep(1);
	return;
}

void WaterLight()
{
	float Brightness = 0.3;
	WS2812BCtrl Ctrl(Brightness);
	const uint32_t led_val = 255 * Brightness;
	uint32_t vals[10];
	LEDPixel leds[4] = {
			{led_val, 0 ,0},
			{0, led_val, 0},
			{0, 0, led_val},
			{led_val, led_val, led_val}
	};

	for (int i = 0; i < 10; ++i)
		vals[i] = 0;
	Ctrl.setSerializedRGB(vals, 0, { 0, 0, 0 });
	Ctrl.setSerializedRGB(vals, 1, { 0, 0, 0 });
	Ctrl.setSerializedRGB(vals, 2, { 0, 0, 0 });
	Ctrl.setSerializedRGB(vals, 3, { 0, 0, 0 });

	int idx = 0;
	while (1)
	{
		++idx;
		Ctrl.setSerializedRGB(vals, 0, leds[idx % 4]);
		Ctrl.setSerializedRGB(vals, 1, leds[(idx + 1) % 4]);
		Ctrl.setSerializedRGB(vals, 2, leds[(idx + 2) % 4]);
		Ctrl.setSerializedRGB(vals, 3, leds[(idx + 3) % 4]);
		Ctrl.OneShot(vals, 9);
	}
}
