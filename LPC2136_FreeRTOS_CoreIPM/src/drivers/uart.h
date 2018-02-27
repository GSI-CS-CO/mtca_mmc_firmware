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
 * uart.h
 *
 *  Created on: Aug 20, 2014
 *      Author: tom
 */

#ifndef UART_H_
#define UART_H_

/**
 * Initialize uart, this function must be called before all other uses of uartx_ functions
 */
void uart_init(void);

/**
 * Put a single character into output buffer, actual transmission is performed
 * Asynchronously within ISR
 */
void uart0_putchar(char c);

/*
 * Returns non-zero if there is data in input buffer
 */
unsigned char uart0_haschar();

/*
 * Returns a single character from uart buffer or 0 if no data is available,
 * note that uart0_haschar should be called before to distinguish between 0 as
 * value and no more data available
 */
char uart0_getchar();

/*
 * Empties RX buffer and drops all contents
 */
void uart0_emptyrx();

/*
 *  Write character to Serial Port 0
 *  (wait until buffer is empty)
 */
int putchar_0( int ch );



#endif /* UART_H_ */
