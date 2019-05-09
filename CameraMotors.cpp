/*
 * I2C.cpp
 *
 *  Created on: Apr 9, 2019
 *      Author: aobog
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
#include <exception>
#include "CameraMotors.h"

#ifdef USING_WIRINGPI
/*Based on the datasheet of SG90 motor
 *1. the PWM period is 20ms (50Hz)
 *2. the range of duty cycle is 1-2ms, the range of PCA9685 is 0-4096 which maps to 0-100% duty cycle,
 *   so the range we could use is 204-409 (5-10%)
 */
#define LEFT_RIGHT_SERVO		0
#define LEFT_RIGHT_MIN			60
#define LEFT_RIGHT_RANGE		150

#define UP_DOWM_SERVO			1
#define UP_DOWN_MIN				100
#define UP_DOWN_RANGE			60

//The I2C address PCA9685 is decided by pin A0-A5 (Fig 4 of PCA9685 Datasheet)
//After checking the AlphaRobot Pi schematic, all of them are connected to ground
//So the address is 0b1000000 -> 0x40, the leading 1 is fixed
const int8_t PCA9685_I2C_ADDRESS = 0x40;
#define SERVO_MOTOR_PWM_FREQ	50

//The osc of PCA9685 is 25MHz
#define PCA9685_OSC_FREQ		25000000.0f
//Register addresses of PCA9685
const int8_t REG_MODE1_ADDR = 0x00;
const int8_t REG_MODE2_ADDR = 0x01;

#define REG_LEDx_ON_LOW_ADDR(x)			(0x06 + ((x) << 2))
#define REG_LEDx_ON_HIGH_ADDR(x)		(0x07 + ((x) << 2))
#define REG_LEDx_OFF_LOW_ADDR(x)		(0x08 + ((x) << 2))
#define REG_LEDx_OFF_HIGH_ADDR(x)		(0x09 + ((x) << 2))

#define REG_LEDALL_ON_LOW_ADDR			0xFA
#define REG_LEDALL_ON_HIGH_ADDR			0xFB
#define REG_LEDALL_OFF_LOW_ADDR			0xFC
#define REG_LEDALL_OFF_HIGH_ADDR		0xFD
#define REG_PRE_SCALE_ADDR				0xFE

void PCA9685PWMFreq(int fd, int freq)
{
	if (freq > 1000)
	{
		std::cout << "Maxium freq is 1000" << std::endl;
		freq = 1000;
	}
	else if (freq < 40)
	{
		std::cout << "Minimum freq is 40" << std::endl;
		freq = 40;
	}

	//Calculate the prescal value, refer to PRE_SCALE (page 25 of PCA9685 datasheet) for more details
	int prescal =(int)(PCA9685_OSC_FREQ / (4096 * freq) + 0.5);

	//According to the foot note at page 13, "Writes to PRE_SCALE register are blocked when SLEEP bit is logic 0 (MODE1)"
	//1. Set the sleep bit
	//2. Set the PRE_SCALE
	//3. Clear the sleep bit
	unsigned char mode1_val = (unsigned char)wiringPiI2CReadReg8(fd, REG_MODE1_ADDR);
	wiringPiI2CWriteReg8(fd, REG_MODE1_ADDR, mode1_val | 0x10);		//Bit 4 of MODE1 register, 1: lower power mode, 0: normal mode
	wiringPiI2CWriteReg8(fd, REG_PRE_SCALE_ADDR, prescal);
	wiringPiI2CWriteReg8(fd, REG_MODE1_ADDR, mode1_val & (~0x10));

	//According to foot node 2 at page 14, "It takes 500us max for the oscillator to be up and running once SLEEP bit has been
	//set to logic 0"
	usleep(1000);
	//Restart all PWM channels, chapter 7.3.1.1 for more details
	wiringPiI2CWriteReg8(fd, REG_MODE1_ADDR, (mode1_val | 0x80) & (~0x10));
}

void PCA9685Control(int fd, int pin, int val)
{
	wiringPiI2CWriteReg16(fd, REG_LEDx_ON_LOW_ADDR(pin), 0x0);
	wiringPiI2CWriteReg16(fd, REG_LEDx_OFF_LOW_ADDR(pin), val & 0x0FFF);
}

void rpiI2CInit()
{
	using namespace std;
	int fd;
	if (-1 == wiringPiSetup())
	{
		cout << "Failed to setup wiringPi!" << endl;
		return;
	}

	fd = wiringPiI2CSetup(PCA9685_I2C_ADDRESS);

	//Enable the auto-increment of registers
	//OI: Is the auto-increment of registers necessary?
	int mode1_val = wiringPiI2CReadReg8(fd, REG_MODE1_ADDR);
	printf("mode1_val: 0x%08x\n", mode1_val);
	wiringPiI2CWriteReg8(fd, REG_MODE1_ADDR, mode1_val | 0x20);

	PCA9685PWMFreq(fd, SERVO_MOTOR_PWM_FREQ);

	//Reset PWM outputs
	wiringPiI2CWriteReg16(fd, REG_LEDALL_ON_LOW_ADDR, 0x0);
	wiringPiI2CWriteReg16(fd, REG_LEDALL_OFF_LOW_ADDR, 0x1000);

	sleep(1);
	int lr_val = LEFT_RIGHT_MIN;
	int ud_val = UP_DOWN_MIN;
	int percentage = 0;
	int increase = 1;
	while(1)
	{
		PCA9685Control(fd, LEFT_RIGHT_SERVO, lr_val);
		PCA9685Control(fd, UP_DOWM_SERVO, ud_val);
		usleep(500000);
		if (increase)
			percentage += 2;
		else
			percentage -= 2;
		if (percentage >= 100)
			increase = 0;
		else if (percentage <= 0)
			increase = 1;
		lr_val = LEFT_RIGHT_MIN + (LEFT_RIGHT_RANGE * percentage) / 100;
		ud_val = UP_DOWN_MIN + (UP_DOWN_RANGE * percentage) / 100;
	}
}
#endif

CameraMotor::CameraMotor(int32_t id, float freq, int32_t max, int32_t min, PCA9685Ctrl &controller)
	: m_id(id),
	  m_freq(freq),
	  m_max(max),
	  m_min(min),
	  m_pwm_controller(&controller)
{

}

CameraMotor::~CameraMotor()
{
	m_pwm_controller = NULL;
}

void CameraMotor::Move(int32_t posn)
{
	assert(posn >= m_min && posn <= m_max);
	assert(m_pwm_controller != NULL);
	m_pwm_controller->UpdatePWMOutput(m_id, (float)posn / 4096);
}

void TwoMotorCtrl()
{
	PCA9685Ctrl camera_motor_ctrl(PCA9685_PIN_SDA, PCA9685_PIN_SCL, PCA9685_I2C_ADDR);
	CameraMotor motor_lr(LEFT_RIGHT_SERVO, CAMERA_MOTOR_PWM_FREQ, LEFT_RIGHT_MIN, LEFT_RIGHT_MAX, camera_motor_ctrl);
	CameraMotor motor_ud(UP_DOWM_SERVO, CAMERA_MOTOR_PWM_FREQ, UP_DOWN_MIN, UP_DOWN_MAX, camera_motor_ctrl);

	//Set the PWM frequency
	camera_motor_ctrl.UpdateFreq(CAMERA_MOTOR_PWM_FREQ);

	//Reset PWM outputs
	camera_motor_ctrl.UpdateAllOutput(0.0);
}
