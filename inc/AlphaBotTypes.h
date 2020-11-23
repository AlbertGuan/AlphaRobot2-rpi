/*
 * AlphaBotTypes.h
 *
 *  Created on: Apr 21, 2019
 *      Author: Albert Guan
 */

#pragma once


#ifndef NULL
#define NULL   ((void *) 0)
#endif

#ifndef MAP_FAILED
#define MAP_FAILED		((void *)(-1))
#endif

#define _In_
#define _Out_
#define _In_opt_
#define _Out_opt_

#define ADDRESS_TO_VOLATILE_POINTER(_addr_) \
    const_cast<volatile uint32_t *>(reinterpret_cast<uint32_t *>(_addr_))
