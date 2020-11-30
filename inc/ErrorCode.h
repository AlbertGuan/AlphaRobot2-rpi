/*
 * ErrorCode.h
 *
 *  Created on: Nov 29, 2020
 *      Author: aobog
 */

#ifndef INC_ERRORCODE_H_
#define INC_ERRORCODE_H_

//
// Highest bit is set if it's an error
//

#define IS_ERROR(_err_)				(((_err) & 0x80000000) != 0)

//
// The operation is succeed
//

#define ERROR_SUCCESS					0x0

//
// Failed to mmap registers
//

#define ERROR_FAILED_MEM_MAP			0x80000001

//
// Unknown Error
//

#define ERROR_UNKNOWN					0x80000002

//
// Invalid pin, e.g. init PWM with a pin which doesn't support PWM.
//

#define ERROR_INVALID_PIN				0x80000003

//
// Channel occupied, e.g. init the PWM channel again which has been occupied.
//

#define ERROR_CHANNEL_OCCUPIED			0x80000004

#endif /* INC_ERRORCODE_H_ */
