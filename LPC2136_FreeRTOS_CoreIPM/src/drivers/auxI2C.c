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
 * auxI2c.c
 *
 *  Created on: Aug 7, 2014
 *      Author: tom
 */

#include <FreeRTOS.h>
#include <task.h>

#include "iopin.h"
#include "lpc21nn.h"
#include "arch.h"
#include "auxI2C.h"
#include "../util/report.h"

const char* AUXI2C_SUBSYSTEM = "AUXI2C";

#define I2C1CONSET_AA (1<<2)
#define I2C1CONSET_SI (1<<3)
#define I2C1CONSET_STO (1<<4)
#define I2C1CONSET_STA (1<<5)
#define I2C1CONSET_EN (1<<6)

unsigned char cb_arg_idx = 0;

void auxI2C_ISR(void) __attribute__ ((interrupt));

void auxI2C_done(unsigned char err) {
//	printf("I2C DONE\n");
//	iopin_clear(P1_19);

	//Data not transmitted, set error
	g_I2C1WS.error = err;
	//Set AA bit, send STOP
	I2C1CONSET = I2C1CONSET_STO;
	//Clear SI flag
	I2C1CONCLR = I2C1CONSET_SI | I2C1CONSET_STA;

	if (g_I2C1WS.error) {
		error(AUXI2C_SUBSYSTEM,"I2C ERROR: 0x%x\n", g_I2C1WS.error);
		return;
	}

	//Free I2C resource
	g_I2C1WS.busy = 0; //no longer busy

}

void auxI2C_ISR() {
	static unsigned char buffer_ptr;

//	iopin_set(P1_19);
	unsigned char stat = I2C1STAT;
//	printf("%x\n",stat);
	switch (stat) {
	case 0x08: //Start issued,
	case 0x10: //Repeated start issued
		//Send slave address
		I2C1DAT = g_I2C1WS.slave_addr;
		//Set AA bit
		I2C1CONSET = I2C1CONSET_AA;
		//Clear SI flag
		I2C1CONCLR = I2C1CONSET_SI;
		//Init isr pointer
		buffer_ptr = 0;
		break;
	case 0x18: //SLA+W ack received
		//Transmit data
		I2C1DAT = g_I2C1WS.data[buffer_ptr++];
		//Set AA bit
		I2C1CONSET = I2C1CONSET_AA;
		//Clear SI flag
		I2C1CONCLR = I2C1CONSET_SI | I2C1CONSET_STA;
		break;
	case 0x20: //SLA+W NACK received
		auxI2C_done(0x20);
		break;
	case 0x28: //Data transmitted and ack received
		//Transmit more data, unless we are on the last byte
		if (buffer_ptr == (g_I2C1WS.data_len)) {
			auxI2C_done(0);
		} else {
			//Transmit data
			I2C1DAT = g_I2C1WS.data[buffer_ptr++];
			//Set AA bit
			I2C1CONSET = I2C1CONSET_AA;
			//Clear SI flag
			I2C1CONCLR = I2C1CONSET_SI;
		}
		break;
	case 0x30: //Dtat transmitted but nack received
		auxI2C_done(0x30);
		break;
	case 0x38: //Arbitration lost
		auxI2C_done(0x38);
		break;
	case 0x40: //SLA+R ACK received
		I2C1CONSET = I2C1CONSET_AA;
		I2C1CONCLR = I2C1CONSET_SI | I2C1CONSET_STA;
		break;
	case 0x48: //SLA+R NACK received
		auxI2C_done(0x48);
		break;
	case 0x50: //Data recived, ACK transmitted
		if (buffer_ptr == g_I2C1WS.data_len) {
			auxI2C_done(0);
		} else {
			g_I2C1WS.data[buffer_ptr++] = I2C1DAT;
			I2C1CONSET = I2C1CONSET_AA;
			I2C1CONCLR = I2C1CONSET_SI;
		}
		break;
	case 0x58: //Data received NACK transmitted
		auxI2C_done(0x58);
		break;
	}

	VICVectAddr = 0x00; /* Acknowledge Interrupt */

}

void auxI2C_init() {
//	printf("DBG: auxI2CInit!\n");

	//Init PINS in iopin_configure
	iopin_dir_out(P0_11); //SCL
	iopin_dir_out(P0_14); //SDA

	iopin_set(P0_11);
	iopin_set(P0_14);

	//reset control register
	I2C1CONCLR = I2C1CONSET_AA | I2C1CONSET_SI | I2C1CONSET_STO | I2C1CONSET_STA
			| I2C1CONSET_EN;

	//Set I2C bus frequency (fPCLK/SCLH+SCLL
	I2C1SCLL = 200;
	I2C1SCLH = 200;

	//Install IRQ handler
	VICIntSelect = 0x0; /* assign all interrupt reqs to the IRQ category */
	VICVectAddr2 = (unsigned long) auxI2C_ISR;
	VICVectCntl2 = 0x20 | IS_I2C1; /* enabled | interrupt source */
	VICIntEnable = IER_I2C1; /* enable I2C interrupts */

	/* ISR address written to the respective address register*/
	I2C1CONSET = I2C1CONSET_EN | I2C1CONSET_AA; /* enabling I2C */

	//I2C is free since there can't be anything in queue
	g_I2C1WS.busy = 0;
	g_I2C1WS.i2c_lock = xSemaphoreCreateMutex();

}


void auxI2C_read(unsigned char address, unsigned char* data,
		unsigned char data_len) {

	//We have to take semaphore
	while(xSemaphoreTake(g_I2C1WS.i2c_lock,10000)!=pdTRUE);

	debug(12,AUXI2C_SUBSYSTEM,"%x",address);


	g_I2C1WS.callback = 0;
	g_I2C1WS.pvt = 0;
	g_I2C1WS.busy = 1;
	//Initialize counter
	//Set up slave address + write bit
	//Set master data counter to match message length
	g_I2C1WS.slave_addr = (address << 1) + 1; //set read address
	g_I2C1WS.data = data;
	g_I2C1WS.data_len = data_len;

	//Start transmit by writing I2C start
	I2C1CONSET = I2C1CONSET_STA;
	//Everything else is done in ISR
	//	I2C1CONCLR = I2C1CONSET_SI;

	while (g_I2C1WS.busy) taskYIELD(); //wait until transaction finishes

	xSemaphoreGive(g_I2C1WS.i2c_lock);
}

void auxI2C_write(unsigned char address, unsigned char* data,
		unsigned char data_len) {

	//We have to take semaphore
	while(xSemaphoreTake(g_I2C1WS.i2c_lock,10000)!=pdTRUE);

	debug(10,AUXI2C_SUBSYSTEM,"write %x\n",address);
	g_I2C1WS.callback = 0;
	g_I2C1WS.pvt = 0;
	g_I2C1WS.busy = 1;
	//Initialize counter
	//Set up slave address + write bit
	//Set master data counter to match message length
	g_I2C1WS.slave_addr = (address << 1); //set write address
	g_I2C1WS.data = data;
	g_I2C1WS.data_len = data_len;

	//Start transmit by writing I2C start
	I2C1CONSET = I2C1CONSET_STA;
	//	I2C1CONCLR = I2C1CONSET_SI;

	while (g_I2C1WS.busy) taskYIELD();
	xSemaphoreGive(g_I2C1WS.i2c_lock);

}



