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

#include <stdio.h>
#include <FreeRTOS.h>
#include <task.h>

#include "uart.h"

#if (__REDLIB_INTERFACE_VERSION__ >= 20000)
// We are using new Redlib_v2 semihosting interface
	#define WRITEFUNC __sys_write
	#define READFUNC __sys_readc
#else
// We are using original Redlib semihosting interface
	#define WRITEFUNC __write
	#define READFUNC __readc
#endif


// Function __write() / __sys_write
//
// Called by bottom level of printf routine within RedLib C library to write
// a character. With the default semihosting stub, this would write the character
// to the debugger console window . But this version writes
// the character to the LPC1768/RDB1768 UART.
int WRITEFUNC (int iFileHandle, char *pcBuffer, int iLength)
{
	taskENTER_CRITICAL();
	unsigned int i;
	for (i = 0; i<iLength; i++)
	{
		putchar_0(pcBuffer[i]);
//		uart0_putchar(pcBuffer[i]); // print each character
	}
	taskEXIT_CRITICAL();
	return iLength;
}
