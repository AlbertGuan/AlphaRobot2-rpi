/*
 * CameraMotor.cpp
 *
 *  Created on: Apr 9, 2019
 *      Author: Albert Guan
 *
 *  This module implements the definition of the camera motor control for
 *  AlphaRobot 2.
 */
#include <iostream>
#include <iomanip>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <bitset>
#include <assert.h>
#include <CameraMotor.h>
#include <exception>

/*Based on the datasheet of SG90 motor
 *1. the PWM period is 20ms (50Hz)
 *2. the range of duty cycle is 1-2ms, the range of PCA9685 is 0-4096 which maps to 0-100% duty cycle,
 *   so the range we could use is 204-409 (5-10%)
 */

CameraMotor::CameraMotor (
	_In_ int32_t PCA9685ChannelId,
	_In_ float PWMFreq,
	_In_ int32_t MinPosition,
	_In_ int32_t MaxPosition,
	_In_ PCA9685Ctrl &Controller
	) : m_PCA9685ChannelId(PCA9685ChannelId),
		m_PWMFreq(PWMFreq),
		m_MinPosition(MinPosition),
		m_MaxPosition(MaxPosition),
		m_PCA9685Controller(&Controller)

/*
 Routine Description:

	This routine is the constructor of Camera Motors, it initializes motor related
	class members and reset the output of PCA9685.

 Parameters:

 	PCA9685ChannelId - Supplies the PCA9685 channel Id which controls this motor.

 	PWMFreq - Supplies the PWM frequency of the motor.

 	MinPosition - Supplies the min position the motor can reach.

 	MaxPosition - Supplies the max position the motor can reach.

 	Controller - Supplies the PCA9685 control instance.

 Return Value:

	None.

*/

{

	//
	// Set the PWM frequency of PCA9685 output which controls this motor.
	//

	m_PCA9685Controller->UpdateFreq(m_PWMFreq);

	//
	// Reset the output to this motor.
	//
	// TODO: Find a proper way to replace this 4096.
	//

	m_PCA9685Controller->UpdatePWMOutput(m_PCA9685ChannelId,
										 (float)MinPosition / 4096);

	return;
}

CameraMotor::~CameraMotor (
	void
	)

/*
 Routine Description:

	This routine is the destructor of Camera Motors.

 Parameters:

 	None.

 Return Value:

	None.

*/

{

	m_PCA9685Controller = NULL;
	return;
}

void
CameraMotor::MoveTo (
	_In_ int32_t Position
	)

/*
 Routine Description:

	This routine moves the motor to target position.

 Parameters:

 	Position - Supplies the position moves the motor to.

 Return Value:

	None.

*/

{

	if (m_PCA9685Controller == NULL) {
		assert(m_PCA9685Controller != NULL);
		goto MoveToEnd;
	}

	if ((Position < m_MinPosition) || (Position > m_MaxPosition)) {
		assert(Position >= m_MinPosition && Position <= m_MaxPosition);
		goto MoveToEnd;
	}

	//
	// Adjust the PWM duty cycle will put the servo motor to corresponding position.
	//

	m_PCA9685Controller->UpdatePWMOutput(m_PCA9685ChannelId,
										 (float)Position / 4096);

MoveToEnd:
	return;
}

void
TwoMotorCtrl (
	void
	)

/*
 Routine Description:

	This is a sample routine which controls two camera motors.

 Parameters:

 	None.

 Return Value:

	None.

*/

{

	//
	// Inits the PCA9685 (PWM controller) before motors.
	//

	RPI_PRINT(InfoLevelDebug, "Init PCA9685 ");
	PCA9685Ctrl PWMController(PCA9685_PIN_SDA,
							  PCA9685_PIN_SCL,
							  PCA9685_I2C_ADDR);

	//
	// Init motors.
	//

	RPI_PRINT(InfoLevelDebug, "Init Motors");
	CameraMotor MotorYaw(YAW_MOTOR_PCA9685_CH_ID,
						 CAMERA_MOTOR_PWM_FREQ,
						 YAW_MOTOR_MIN,
						 YAW_MOTOR_MAX,
						 PWMController);

	CameraMotor MotorPitch(PITCH_MOTOR_PCA9685_CH_ID,
						   CAMERA_MOTOR_PWM_FREQ,
						   PITCH_MOTOR_MIN,
						   PITCH_MOTOR_MAX,
						   PWMController);

	sleep(1);
	int YawPosition = YAW_MOTOR_MIN;
	int PitchPosition = PITCH_MOTOR_MIN;
	int percentage = 0;
	int increase = 1;
	while(1) {
		MotorYaw.MoveTo(YawPosition);
		MotorPitch.MoveTo(PitchPosition);
		usleep(500000);
		if (increase) {
			percentage += 2;

		} else {
			percentage -= 2;
		}

		if (percentage >= 100) {
			increase = 0;

		} else if (percentage <= 0) {
			increase = 1;
		}

		YawPosition = YAW_MOTOR_MIN + (YAW_MOTOR_RANGE * percentage) / 100;
		PitchPosition = PITCH_MOTOR_MIN + (PITCH_MOTOR_RANGE * percentage) / 100;
	}

	return;
}
