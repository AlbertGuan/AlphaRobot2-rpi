/*
 * AlphaRobotConstants.h
 *
 *  Created on: Nov 26, 2020
 *      Author: Albert Guan
 */

#ifndef ALPHAROBOTCONSTANTS_H_
#define ALPHAROBOTCONSTANTS_H_

//
// The I2C address PCA9685 is decided by pin A0-A5 (Fig 4 of PCA9685 Datasheet)
// After checking the AlphaRobot Pi schematic, all of them are connected to ground
// So the address is 0b1000000 -> 0x40, the leading 1 is fixed
//

#define PCA9685_I2C_ADDR			0x40

//
// GPIO Pin number of I2C inputs to PCA9685. Check the schematic for
// more details.
//

#define PCA9685_PIN_SDA				2
#define PCA9685_PIN_SCL				3

//
// From the datasheet of SG90 motor, its control frequency is 50Hz
//

#define CAMERA_MOTOR_PWM_FREQ		50.0f

//
// Channel ID of PCA9685 which connects to the yaw motor. Check the schematic for
// more details.
//

#define YAW_MOTOR_PCA9685_CH_ID		0
#define YAW_MOTOR_MIN				60
#define YAW_MOTOR_RANGE				150
#define YAW_MOTOR_MAX				(YAW_MOTOR_MIN + YAW_MOTOR_RANGE)

//
//Channel ID of PCA9685 which connects to the pitch motor. Check the schematic for
// more details.
//

#define PITCH_MOTOR_PCA9685_CH_ID	1
#define PITCH_MOTOR_MIN				100
#define PITCH_MOTOR_RANGE			60
#define PITCH_MOTOR_MAX				(PITCH_MOTOR_MIN + PITCH_MOTOR_RANGE)

#endif /* ALPHAROBOTCONSTANTS_H_ */
