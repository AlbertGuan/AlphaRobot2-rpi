/*
 * Diag.h
 *
 *  Created on: Apr 23, 2019
 *      Author: Albert Guan
 */

#ifndef DIAG_H_
#define DIAG_H_

typedef enum _InfoLevel_ {
	InfoLevelDebug = 0,
	InfoLevelInfo,
	InfoLevelWarning,
	InfoLevelError,
	InfoLevelCritical,
	InfoLevelMax = InfoLevelCritical
} InfoLevel, *PInfoLevel;

#if DBG

#define RPI_PRINT_LEVEL		InfoLevelDebug

#else

#define RPI_PRINT_LEVEL		InfoLevelWarning

#endif

void
RpiPrint (
	InfoLevel Level,
	const char *FuncName,
	int line,
	const char *fmt,
	...
	);

#define RPI_PRINT(level, msg) \
	RpiPrint(level, __func__, __LINE__, msg)

#define RPI_PRINT_EX(level, msg, ...) \
	RpiPrint(level, __func__, __LINE__, msg, __VA_ARGS__)

#endif /* DIAG_H_ */
