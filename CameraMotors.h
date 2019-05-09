/*
 * I2C.h
 *
 *  Created on: Apr 9, 2019
 *      Author: aobog
 */

#ifndef CAMERAMOTORS_H_
#define CAMERAMOTORS_H_
#include <wiringPi.h>
#include <wiringPiI2C.h>

void rpiI2CInit();
void TwoMotorCtrl();
#endif /* CAMERAMOTORS_H_ */
