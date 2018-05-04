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

#include "payload.h"
#include "drivers/mmcio.h"
#include "drivers/iopin.h"
#include "util/report.h"


//CoreIPM Includes
#include "coreIPM/ipmi.h"
#include "coreIPM/mmc.h"
#include "coreIPM/amc.h"

//FreeRTOS includes
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

const char* PAYLOAD_SUBSYSTEM = "PAYLOAD";
const char* PAYLOAD_FSM = "PYLD_FSM";


/* Forward declarations */
void tsk_payload_fsm();
void tsk_payload_monitor();

/* Globals */

void payload_init() {
	g_module_state.payload_state = PAYLOAD_IDLE;

	//activate payload monitor
	xTaskCreate(tsk_payload_monitor, "PYLD_MTR", configMINIMAL_STACK_SIZE, 0,
			tskIDLE_PRIORITY+2, 0);

	//start payload fsm
	xTaskCreate(tsk_payload_fsm, "PYLD_FSM", configMINIMAL_STACK_SIZE, 0,
			tskIDLE_PRIORITY+2, 0);
}

void payload_activate() {
	taskENTER_CRITICAL();

	g_module_state.payload_activate = 1;

	//check if FSM is in error state and reset it
	if (g_module_state.payload_state == PAYLOAD_ERR) {
		enterIdle();
	}

	taskEXIT_CRITICAL();
}

void payload_quiesce() {

	taskENTER_CRITICAL();

	//If payload is active we need to shut it down nicely
	if (g_module_state.payload_state == PAYLOAD_ACTIVE            ||
		g_module_state.payload_state == PAYLOAD_ACTIVE_PCIE_RESET ||
		g_module_state.payload_state == PAYLOAD_ACTIVE_PCIE_RESET_DEASSERT ||
		g_module_state.payload_state == PAYLOAD_ACTIVE_BLINK_GREEN ) {

		info(PAYLOAD_SUBSYSTEM, "Shutting down payload");
		info(PAYLOAD_SUBSYSTEM, "Waiting for quiesce");
		g_module_state.payload_state = PAYLOAD_WAIT_SHUTDOWN;
	} else if (g_module_state.payload_state == PAYLOAD_ERR) {
		//do nothing if we are in error state
		mmc_hot_swap_state_change( MODULE_QUIESCED);
	}
	//otherwise we can simply reset the FSM
	else {
		enterIdle();
	}

	taskEXIT_CRITICAL();
}

void enterError(char* reason) {
	error(PAYLOAD_SUBSYSTEM, reason);
	//Set discrete sensor error

	g_module_state.payload_activate = 0;
	g_module_state.payload_state = PAYLOAD_ERR;
	mmc_hot_swap_state_change( MODULE_QUIESCED);

}

void enterIdle() {

	//On enter idle
	g_module_state.payload_state = PAYLOAD_IDLE;
	//notify about transition to M1
	mmc_hot_swap_state_change( MODULE_QUIESCED);
}

void tsk_payload_fsm() {

//	printf("INFO[PAYLOAD]: Starting FSM task!\n");
	info(PAYLOAD_SUBSYSTEM, "Starting FSM task!");

	static int fsm_timeout;
	int green_led_state;

	while (1) {
		/* FSM evaluation should not be preempted since FSM state can get change
		 * from another thread in meanwhile (e.g. due to a handle state change)
		 */
		taskENTER_CRITICAL();

		switch (g_module_state.payload_state) {
		case PAYLOAD_IDLE:
			//Set LEDs
			iopin_led(LED_ERROR,0,0);
			iopin_led(LED_OK,0,0);

			//Set outputs
			iopin_clear(PAYLOAD_PIN_POWER);
			iopin_clear(PAYLOAD_PIN_QUISCE_OUT);
			iopin_clear(PAYLOAD_PIN_FPGA_CONFIG);

			//Activation request or when outside,
			// first we wait for 12V Payload power to come up
			if (g_module_state.payload_activate || g_module_state.host_type == AMC_FTRN_OUTSIDE) {
				info(PAYLOAD_FSM,	"Payload activation start");
				info(PAYLOAD_FSM,	"Waiting for 12V");
				g_module_state.payload_state = PAYLOAD_WAIT_FOR_12V;
				g_module_state.payload_activate = 0;
			}

			break;
		case PAYLOAD_WAIT_FOR_12V:
			//Set LEDs
			iopin_led(LED_ERROR,0,0);
			iopin_led(LED_OK,0,0);

			//Set outputs
			iopin_clear(PAYLOAD_PIN_POWER);
			iopin_clear(PAYLOAD_PIN_QUISCE_OUT);
			iopin_clear(PAYLOAD_PIN_FPGA_CONFIG);

			if (g_module_state.sensors.PP12V > 10000) {
				info(PAYLOAD_FSM, "Enabling payload power");
				//Wait for power good
				g_module_state.payload_state = PAYLOAD_WAIT_FOR_PG;
				//Set timeout
				fsm_timeout = 10;
			}
			break;
		case PAYLOAD_WAIT_FOR_PG:
			//Set LEDs
			iopin_led(LED_ERROR,0,0);
			iopin_led(LED_OK,0,0);
			//Set outputs:
			iopin_set  (PAYLOAD_PIN_POWER);
			iopin_clear(PAYLOAD_PIN_QUISCE_OUT);
			iopin_clear(PAYLOAD_PIN_FPGA_CONFIG);

			if (g_module_state.sensors.POWERGOOD) {
				info(PAYLOAD_FSM, "Power Good Asserted");
				info(PAYLOAD_FSM, "Enable FPGA boot");

				g_module_state.payload_state = PAYLOAD_WAIT_FOR_CONF_DONE;
				//Set timeout
				fsm_timeout = 60;
			}

			/*
			 * Error handling
			 */
			if (g_module_state.sensors.PP12V < 10000) {
				iopin_clear(PAYLOAD_PIN_POWER);
				enterError("Unexpected payload voltage drop!\n");
			}

			if (!fsm_timeout--) {
				enterError("Powergood timeout!");
			}
			break;

		case PAYLOAD_WAIT_FOR_CONF_DONE:
			//Set LEDs
			iopin_led(LED_ERROR,0,0);
			iopin_led(LED_OK,0,0);
			//Set outputs:
			iopin_set  (PAYLOAD_PIN_POWER);
			iopin_clear(PAYLOAD_PIN_QUISCE_OUT);
			iopin_set  (PAYLOAD_PIN_FPGA_CONFIG);

			// normally wait for CONF_DONE but when outside on AUX power,
			// continue to active state even if CONF_DONE not asserted
			if (g_module_state.sensors.CONF_DONE || iopin_get(STANDALONE)) {
				info(PAYLOAD_FSM, "CONF_DONE Asserted");
				info(PAYLOAD_FSM, "Payload active");

				//Set flash iopin
				g_module_state.payload_state = PAYLOAD_ACTIVE;
				//Set timeout
				fsm_timeout = 120;
			}

			/*
			 * Error handling
			 */
			if (g_module_state.sensors.PP12V < 10000) {
				iopin_clear(PAYLOAD_PIN_POWER);
				enterError("Unexpected payload voltage drop!\n");
			}

			if (!fsm_timeout--) {
				enterError("Conf Done timeout!");
			}
			break;

		case PAYLOAD_ACTIVE:
			fsm_timeout = 60; //quiesce timeout

			//Set LEDs
			iopin_led(LED_ERROR,0,0);
			iopin_led(LED_OK,0,1);
			//set outputs:
			iopin_set  (PAYLOAD_PIN_POWER);
			iopin_clear(PAYLOAD_PIN_QUISCE_OUT);
			iopin_set  (PAYLOAD_PIN_FPGA_CONFIG);

			// when in Libera and PCIe reset command was received
			if (g_module_state.cmd == FTRN_CMD_PCIE_RESET ){
				g_module_state.payload_state = PAYLOAD_ACTIVE_PCIE_RESET;
			}else if (hot_swap_handle_last_state == MODULE_HANDLE_OPENED){
				g_module_state.payload_state = PAYLOAD_ACTIVE_BLINK_GREEN;
			}

			/*
			 * Error handling
			 */
			if (g_module_state.sensors.PP12V < 10000) {
				iopin_clear(PAYLOAD_PIN_POWER);
				enterError("Unexpected payload voltage drop!\n");
			}

			break;

		// Libera specific
		case PAYLOAD_ACTIVE_PCIE_RESET:
			//Set LEDs
			iopin_led(LED_ERROR,0,0);
			iopin_led(LED_OK,0,0);
			//set outputs:
			iopin_set  (PAYLOAD_PIN_POWER);
			iopin_clear(PAYLOAD_PIN_QUISCE_OUT);
			iopin_set  (PAYLOAD_PIN_FPGA_CONFIG);

			info(PAYLOAD_FSM, "Asserting PCIe reset");
			iopin_clear(PAYLOAD_PIN_PCIE_RESET);
			g_module_state.cmd = 0;
			g_module_state.payload_state = PAYLOAD_ACTIVE_PCIE_RESET_DEASSERT;

			/*
			 * Error handling
			 */
			if (g_module_state.sensors.PP12V < 10000) {
				iopin_clear(PAYLOAD_PIN_POWER);
				enterError("Unexpected payload voltage drop!\n");
			}

			break;

		// Libera specific
		case PAYLOAD_ACTIVE_PCIE_RESET_DEASSERT:

			//Set LEDs
			iopin_led(LED_ERROR,0,0);
			iopin_led(LED_OK,0,1);
			//set outputs:
			iopin_set  (PAYLOAD_PIN_POWER);
			iopin_clear(PAYLOAD_PIN_QUISCE_OUT);
			iopin_set  (PAYLOAD_PIN_FPGA_CONFIG);

			info(PAYLOAD_FSM, "De-asserting PCIe reset");
			iopin_set(PAYLOAD_PIN_PCIE_RESET);
			g_module_state.payload_state = PAYLOAD_ACTIVE;
			info(PAYLOAD_FSM, "Payload active");

			/*
			 * Error handling
			 */
			if (g_module_state.sensors.PP12V < 10000) {
				iopin_clear(PAYLOAD_PIN_POWER);
				enterError("Unexpected payload voltage drop!\n");
			}

			break;

		case PAYLOAD_ACTIVE_BLINK_GREEN:
			//Set LEDs
			// blink green LED
			green_led_state = iopin_get(LED_OK);
			iopin_led(LED_OK,0,green_led_state);

			iopin_led(LED_ERROR,0,0);

			//set outputs:
			iopin_set(PAYLOAD_PIN_POWER);
			iopin_set(PAYLOAD_PIN_QUISCE_OUT);
			iopin_set(PAYLOAD_PIN_FPGA_CONFIG);

			// if user changed his mind and closed handle
			// before module was quiesced
			if (hot_swap_handle_last_state == MODULE_HANDLE_CLOSED){
				info(PAYLOAD_FSM, "Payload active");
				g_module_state.payload_state = PAYLOAD_ACTIVE;
			}

			/*
			 * Error handling
			 */
			if (g_module_state.sensors.PP12V < 10000) {
				iopin_clear(PAYLOAD_PIN_POWER);
				enterError("Unexpected payload voltage drop!\n");
			}

			break;

		case PAYLOAD_WAIT_SHUTDOWN:
			info(PAYLOAD_FSM, "Waiting for shutdown...");
			//Set LEDs

			// blink green LED
			green_led_state = !iopin_get(LED_OK);
			iopin_led(LED_OK,0,green_led_state);

			iopin_led(LED_ERROR,0,0);

			//set outputs:
			iopin_set(PAYLOAD_PIN_POWER);
			iopin_set(PAYLOAD_PIN_QUISCE_OUT);
			iopin_set(PAYLOAD_PIN_FPGA_CONFIG);


			if (g_module_state.sensors.QUIESCE) {
				info(PAYLOAD_FSM, "Payload quiesced");
				enterIdle();
			}

			if (!fsm_timeout--) {
				enterError("Quiesce in timeout!");
			}
			break;

		case PAYLOAD_ERR:
			//Set LEDs
			iopin_led(LED_ERROR,0,1);
			iopin_led(LED_OK,0,0);
			//set outputs:
			iopin_clear(PAYLOAD_PIN_POWER);
			iopin_clear(PAYLOAD_PIN_QUISCE_OUT);
			iopin_clear(PAYLOAD_PIN_FPGA_CONFIG);

			break;
		}

		taskEXIT_CRITICAL();
		//Evaulate FSM again in 100ms;
		vTaskDelay(100);
	}

	error(PAYLOAD_FSM, "FSM task exited!");
}

/*
 * Return voltage on AI<channel> pin in millivolts
 */
//TODO: add adc thread safety
unsigned int readAdc(unsigned char channel) {
	AD0CR = 0xff | 		//Sample all 7 input pins
			(5 << 8) | 	//CLKDIV = 5, 2,5Mhz sample rate;
			(1 << 16) | 	//Burst = 1
			(0 << 17) | 	//CLKS == 11 clocks
			(1 << 21) |	//PDN==1, enable AD
			(0 << 24);	//Start == 0, burst start

	//Wait for conversion to complete
	while (!(AD0GDR & (1 << 31))) {
		taskYIELD();
	}

	unsigned int res = (*((volatile unsigned int*) (0xE0034010 + channel * 4))
			>> 6) & 0xfff;
	return (res * 29) / 10;

}

/**
 * Function is periodically executed and monitors MP 3V3 and PP 12V voltage levels.
 * Once 12V voltage rises above 12V it enables payload power and fires up startup
 * procedure
 */
void tsk_payload_monitor() {
	info(PAYLOAD_SUBSYSTEM, "Monitor task started");
	while (1) {
		int pp12v = (readAdc(2) * 512) / 100; //12V goes over voltage divider, this is a recalc
		int mp3v3 = (readAdc(3) * 136) / 100; //3V3 goes over voltage divider, this is a recalc

		//Analog reads
		g_module_state.sensors.PP12V = pp12v;
		g_module_state.sensors.MP3V3 = mp3v3;

		//Discrete inputs
		g_module_state.sensors.POWERGOOD   = iopin_get(PAYLOAD_PIN_POWER_GOOD);
		g_module_state.sensors.QUIESCE     = iopin_get(PAYLOAD_PIN_QUISCE_IN);
		g_module_state.sensors.CONF_DONE   = iopin_get(PAYLOAD_PIN_CONF_DONE);
		g_module_state.sensors.FPGA_CONFIG = iopin_get(PAYLOAD_PIN_FPGA_CONFIG);
		g_module_state.sensors.TRIG_LIBERA = iopin_get(PAYLOAD_PIN_IN_SLOT8);
		g_module_state.sensors.TRIG_MTCA4  = iopin_get(PAYLOAD_PIN_MTCA4_EN);
		g_module_state.sensors.JTAGSW_POS  = iopin_get(PAYLOAD_PIN_JTAGSW_POS);

		vTaskDelay(10);
	}
	error(PAYLOAD_SUBSYSTEM, "Monitor task exited\n");

}

void payload_port_set_state(unsigned char link_type, char *link_type_name,unsigned char port,unsigned char state){
	info("PAYLOAD","Set %s port %d state to",link_type_name,port);
	info("PAYLOAD","\t%s",state?"ENABLED":"DISABLED");

	switch (link_type) {
		case AMC_LINK_PCI_EXPRESS:  // PCIe
			g_module_state.ports.PCIe = state;
	        if(state){
	        	iopin_set(PAYLOAD_PIN_PCIE_RESET);
	        }else{
	        	iopin_clear(PAYLOAD_PIN_PCIE_RESET);
	        }
			break;
		case FTRN_LIBERA_TRIGGER:  // Libera triggers on PORTS 6-7
			g_module_state.ports.LiberaTrig = state;
	        if(state){
	    		iopin_set(PAYLOAD_PIN_IN_SLOT8);
	        }else{
	    		iopin_clear(PAYLOAD_PIN_IN_SLOT8);
	        }
			break;
		default:
			info("PAYLOAD","Not FTRN port!");
			break;
	}


	if (state) {
		iopin_set(LED2_RED);
		/* The following is here to handle graceful start of FRU with hot-swap handle pushed in.
		 * Set delayed actions on port state Enable here
		 */
		if (g_module_state.payload_state == PAYLOAD_IDLE) {
			payload_activate();
		}
	} else {
		iopin_clear(LED2_RED);
	}
}


