/*
 * I2C.h
 *
 *  Created on: Apr 9, 2019
 *      Author: aobog
 */

#pragma once
#ifdef USING_WIRINGPI
#include <wiringPi.h>
#include <wiringPiI2C.h>
#endif
#include "PCA9685Ctrl.h"

#ifdef USING_WIRINGPI
void rpiI2CInit();
#endif

void TwoMotorCtrl();

class CameraMotor
{
public:
	CameraMotor(int32_t id, float freq, int32_t min, int32_t max, PCA9685Ctrl &controller);
	~CameraMotor();
	void SetController(PCA9685Ctrl &controller);
	void Move(int32_t posn);
private:
	int32_t m_id;					//Which channel the motor is connected to on PCA9685
	float m_freq;
	int32_t m_min;
	int32_t m_max;
	PCA9685Ctrl *m_pwm_controller;
};
