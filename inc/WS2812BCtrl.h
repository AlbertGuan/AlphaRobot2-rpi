/*
 * WS2812BCtrl.h
 *
 *  Created on: Apr 14, 2019
 *      Author: Albert Guan
 */

#pragma once

#include <wiringPi.h>
#include <stdint.h>
#include "AlphaBotTypes.h"
#include "GpioPwm.h"
#include "Rpi3BConstants.h"

/*
 * The WS2812B LED takes 24 "bits" (G, R, B) to control the color and brightness
 * We have 4 WS2812B LEDs on the robot, so we need 4 * 24 = 96"bits" to control all of them
 * After sending these 96"bits", the pin should be reset for >= 50us indicates control done
 * We are using the PWM serializer mode to send these "bits", base on the datasheet to WS2812B
 * each "bits" lasts 1.25us (8MHz):
 * 		- logical 1 is 0.8us high and 0.45us low
 * 		- logical 0 is 0.4us high and 0.85us low
 * So we may use 1 PWM pulse to indicate 1 "bit", and each logical bit lasts about 1.25 / 3 us (2.4MHz)	=> WS2812B_PWM_DIVIDOR
 * The PWM serializer mode outputs PWM wave by bits, the "RNG1/2" register indicates who wide each word represents => WS2812B_PWM_RANGE
 * We use 24 * 4 * 3 /32 = 9 words in the FIFO to control these 4 LEDs.
 * ToDo:
 * Using DMA to update the FIFO
 */
class WS2812BCtrl
{
public:

	WS2812BCtrl (
		_In_ float brightness = 0.3
		);

	~WS2812BCtrl (
		void
		);

	void
	SetBrightness (
		_In_ float Brightness
		);

	void
	setSerializedRGB (
		_Out_ uint32_t *arr,
		_In_ const int led_idx,
		_In_ const LEDPixel &color
		);

	void
	OneShot (
		_In_ uint32_t *vals,
		_In_ uint32_t len
		);

private:
	static const uint32_t BITS_PER_COLOR = 8;
	static const uint32_t WS2812B_PWM_RANGE = 32;
	static const uint32_t WS2812B_PWM_DIVIDOR = 8;		//19.2MHz / 8 = 2.4MHz
	static const uint32_t WS2812B_PWM_MODE = 1;			//Seriliser mode
	static const uint32_t WS2812B_PWM_FIFO = 1;			//Using FIFO

	GpioPwm *m_PWM;
	float m_Brightness;
};

//
// Sample code
//

void WaterLight();
