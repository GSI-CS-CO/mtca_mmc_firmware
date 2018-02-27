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
 * m24128.c
 *
 *  Created on: Aug 19, 2014
 *      Author: tom
 */


#include <stdio.h>
#include <string.h>
#include "auxI2C.h"
#include "m24eeprom.h"

void m24eeprom_init(struct m24eeprom_ws* ws, unsigned char address){
	ws->address=address;
}


void m24eeprom_read(struct m24eeprom_ws* ws, unsigned int address, unsigned char len){
	ws->buffer[0] = (address >> 8) & 0xff;
	ws->buffer[1] = (address ) & 0xff;
	auxI2C_write(ws->address,ws->buffer,2); //Write address

	memset(ws->buffer,0,5);
	auxI2C_read(ws->address,ws->buffer,3);

	printf("Received data %x %x %x\n",ws->buffer[0],ws->buffer[1],ws->buffer[2]);
}


void m24eeprom_write(struct m24eeprom_ws* ws, unsigned int address, unsigned char* data, unsigned char len){
	ws->buffer[0] = (address >> 8) & 0xff;
	ws->buffer[1] = (address ) & 0xff;
	auxI2C_write(ws->address,ws->buffer,2); //Write address

	memcpy(ws->buffer,data,len);
	auxI2C_write(ws->address,ws->buffer,3);

	printf("Received data %x %x %x\n",ws->buffer[0],ws->buffer[1],ws->buffer[2]);
}
