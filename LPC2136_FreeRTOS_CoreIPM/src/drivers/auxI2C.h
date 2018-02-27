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

#include "FreeRTOS.h"
#include "semphr.h"

//auxI2C work structure, holds current data
struct auxI2C_ws {
	SemaphoreHandle_t i2c_lock;
	unsigned char slave_addr;
	unsigned char* data;
	unsigned char data_len;
	unsigned char error;
	unsigned char busy;
	void(*callback)(void*);
	void* pvt;
} g_I2C1WS;


void auxI2C_init();
void auxI2C_read(unsigned char address, unsigned char* data, unsigned char data_len);
void auxI2C_write(unsigned char address, unsigned char* data, unsigned char data_len);

