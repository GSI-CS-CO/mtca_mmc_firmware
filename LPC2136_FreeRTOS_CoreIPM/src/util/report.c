/*
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   @license GPL-3.0+ <http://spdx.org/licenses/GPL-3.0+>
 */

/*
 * report.c
 *
 *  Created on: Aug 22, 2014
 *      Author: tom
 */

#include <stdio.h>
#include <stdarg.h>
#include "../drivers/iopin.h"
#include "../drivers/mmcio.h"
#include "report.h"

#define BUFFER_LENGTH 28
static unsigned int interest_level=0;

void report_init(int level) {
	interest_level = level;
}

void error(const char* subsystem, const char* format, ...) {
	char txt[BUFFER_LENGTH];

	va_list args;
	va_start(args, format);

	printf("ERROR[%s]:", subsystem);
	vsnprintf(txt, sizeof(txt), format, args);
	printf(txt);
	printf("\n");

	va_end(args);
	iopin_set(LED1_RED);
}

void info(const char* subsystem, const char* format, ...) {
	char txt[BUFFER_LENGTH];

	va_list args;
	va_start(args, format);

	printf("INFO[%s]:", subsystem);
	vsnprintf(txt, sizeof(txt), format, args);
	printf(txt);
	printf("\n");

	va_end(args);
}

void debug(int level, const char* subsystem, const char* format, ...) {
	if ( interest_level > level) {
		char txt[BUFFER_LENGTH];

		va_list args;
		va_start(args, format);

		printf("DEBUG[%s]:", subsystem);
		vsnprintf(txt, sizeof(txt), format, args);
		printf(txt);
		printf("\n");

		va_end(args);
	}
}

