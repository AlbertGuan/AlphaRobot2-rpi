/*
 * CameraMotor.h
 *
 *  Created on: Apr 9, 2019
 *      Author: Albert Guan
 *
 *  This module implements the declaration of the camera motor control for
 *  AlphaRobot 2.
 */

#pragma once

#include "PCA9685Ctrl.h"

void
TwoMotorCtrl (
	void
);

/*
 * Overview
 * 	There are two SG90 servo motors on the robot to control the position of the
 * 	camera. Instead of using the GPIO PWM outputs of the RPI, the robot uses the
 * 	PCA9685 to control these motors.
 *
 * 	RPI -(I2C)-> PCA9685 -(PWM_0)-> Yaw Motor
 * 					     -(PWM_1)-> Pitch Motor
 *
 * 	The RPI controls the output of PCA9685 through I2C, and the PCA9685 sends
 * 	PWM signals to motors to control their positions.
 *
 * SG50 Motor
 * 	The control frequency is 50Hz -> Output of PCA9685 should be 50Hz
 * 	The position of the motor is controled by the duty cycle of the PWM.
 * 	-> 7.5% is middle, 10% is right most, 5% is left most.
 *
 * PCA9685
 * 	Check PCA9685Ctrl module for more details.
 *
 * I2C
 * 	Check GpioI2C module for more details.
 *
 */

class CameraMotor
{

public:

	CameraMotor (
		_In_ int32_t PCA9685ChannelId,
		_In_ float PWMFreq,
		_In_ int32_t MinPosition,
		_In_ int32_t MaxPosition,
		_In_ PCA9685Ctrl &Controller
	);

	~CameraMotor (
		void
	);

	void
	MoveTo (
		_In_ int32_t Position
	);

private:

	//
	// Which channel the motor is connected to on PCA9685
	//

	int32_t m_PCA9685ChannelId;
	float m_PWMFreq;
	int32_t m_MinPosition;
	int32_t m_MaxPosition;
	PCA9685Ctrl *m_PCA9685Controller;
};
