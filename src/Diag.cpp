/*
 * Diag.cpp
 *
 *  Created on: Apr 23, 2019
 *      Author: Albert Guan
 */

#include <Diag.h>
#include <iostream>
#include <cstdarg>

void
RpiPrint (
	InfoLevel Level,
	const char *FuncName,
	int line,
	const char *fmt,
	...
	)

{

	va_list arg;
	char buffer[256] = {0};
	int offset;

	if (Level >= RPI_PRINT_LEVEL) {
		offset = sprintf(buffer, "%s(%d): ", FuncName, line);
		va_start(arg, fmt);
		offset += vsprintf(&buffer[offset], fmt, arg);
		sprintf(&buffer[offset], "\n");
		printf(buffer);
		va_end(arg);
	}

	return;
}
