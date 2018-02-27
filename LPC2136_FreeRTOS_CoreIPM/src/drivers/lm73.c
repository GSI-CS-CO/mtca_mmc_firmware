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
 * lm73.c
 *
 *  Created on: Aug 8, 2014
 *      Author: tom
 */
#include "auxI2C.h"
#include "lm73.h"
#include "../util/report.h"

const char* LM73_SUBSYSTEM = "LM73";

void lm73_init(struct lm73_ws* ws) {

	debug(1,LM73_SUBSYSTEM,"Init %x", ws->address);

	ws->buffer[0]=0x4; //pointer to control status register
	ws->buffer[1]=0x60; //set csr
	auxI2C_write(ws->address,ws->buffer,2);

	ws->buffer[0]=0;
	auxI2C_write(ws->address,ws->buffer,1);

	debug(1,LM73_SUBSYSTEM,"Sensor 0x%x Init Done", ws->address);
}

void lm73_readTemp(struct lm73_ws* ws) {
	auxI2C_read(ws->address,ws->buffer,2);
	int temp = ((ws->buffer[0] << 8) | (ws->buffer[1])) >> 2;
	/*
	 * Convert temperatures to mDeg [milli degrees C]
	 */
	temp *= 1000;
	temp /= 32;
	ws->last_reading=temp;
	//	printf("LM73[%x]: temp %d.%d %d\n", ws->address,temp / 1000, temp - (temp / 1000) * 1000, ws->read_complete++);
	//debug(1, LM73_SUBSYSTEM, "w0:%d w1:%d t:%d", ws->buffer[0], ws->buffer[1], temp);
	//debug(1, LM73_SUBSYSTEM, "temp %d %x\n", temp, temp);
}

