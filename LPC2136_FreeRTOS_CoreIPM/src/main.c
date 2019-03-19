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

#include "util/report.h"

/* Drivers and periph support code */
#include "drivers/lpc21nn.h"
#include "drivers/iopin.h"
#include "drivers/uart.h"
#include "drivers/auxI2C.h"
#include "drivers/m24eeprom.h"
#include "drivers/lm73.h"
#include "drivers/mmcio.h"

/* FreeRTOS includes */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"
#include "timers.h"

/* CoreIPM Includes */
#include "coreIPM/timer.h"
#include "coreIPM/ws.h"
#include "coreIPM/i2c.h"
#include "coreIPM/picmg.h"
#include "coreIPM/module.h"
#include "coreIPM/amc.h"
#include "coreIPM/debug.h"

/* from src */
#include "payload.h"
#include "mmc_config.h"
#include "build_id.h"
#include "project_defs.h"


#define HELP_TEXT "\
\n\nAvailable MMC Console commands:\n\
R0-9 : set DEBUG Report level\n\
i,I : print MMC build info\n\
p,P : disable/enable PCIe port\n\
l,L : MMC LED test - leds OFF/ON\n\
o   : exit MMC LED test\n\
\n--- Debug tools --------------\n\
t   : display task list with status and stack info\n\
q   : check IPMI bus I2C pin state\n\
D   : change IPMI bus I2C pins to GPIO inputs\n\
d   : change IPMI bus I2C pins to I2C\n\
e,E : disable/enable I2C controller\n\
r   : reset IPMI bus I2C controller\n\
b,B : set I2C0 SCL frequency to 60k/100k\n\
w   : print ws_array states\n\
u   : unclog ws array\n\
s   : list I2C0 activity log\n\
c   : print out CQ array\n\
\n\
"


SemaphoreHandle_t slowTask_sem;
struct m24eeprom_ws g_M24EEPROM = { .address = 0b1010100};

unsigned report_set = 0;

/**
 * This task simulates coreIPM Timer interrupt, the only thing it does
 * is it increments internal tick counter that is used by timer
 * callbacks...
 */
static void tskCoreIPM_hardclock(){
	extern unsigned long lbolt;
	while(1){
		lbolt++;
		vTaskDelay(100);
	}
}


// Notify user that MMC is aware of the host type
// and indicate to FPGA that we are in Libera
static void tskHostType(){
	unsigned int on_delay, off_delay;
	char c;
	char statusmsg[400];

	// default state
	g_module_state.host_type     = AMC_FTRN_HOST_NOT_EVALUATED;
  g_module_state.ipmi_amc_host = AMC_FTRN_HOST_NOT_EVALUATED;


	info("HOST_TYPE","Init: host_type=%X", g_module_state.host_type);
	on_delay  = 250;
	off_delay = 250;
	iopin_clear(PAYLOAD_PIN_IN_LIBERA);

	// wait until in active state
	// while(g_module_state.payload_state != PAYLOAD_ACTIVE);

	while(1){
		// Check host type when payload in active state

		if (g_module_state.payload_state == PAYLOAD_ACTIVE            ||
			g_module_state.payload_state == PAYLOAD_ACTIVE_PCIE_RESET ||
			g_module_state.payload_state == PAYLOAD_ACTIVE_PCIE_RESET_DEASSERT ||
			g_module_state.payload_state == PAYLOAD_ACTIVE_BLINK_GREEN ) {

			// determine host type, check if amc is in the crate and which one

			if (iopin_get(STANDALONE)) { // outside on AUX power
				if(g_module_state.host_type != AMC_FTRN_OUTSIDE){
					debug(3,"HOST_TYPE","Change: %X >> %X", g_module_state.host_type,AMC_FTRN_OUTSIDE);
					info("HOST_TYPE","FTRN OUTSIDE");
					g_module_state.host_type = AMC_FTRN_OUTSIDE;
					on_delay  = 100;
					off_delay = 400;
					// allow enabling of Libera triggers by SW
					iopin_clear(PAYLOAD_PIN_IN_LIBERA);
					// enable PCIe
					info("INFO","Enabling PCIe port");
					payload_port_set_state(AMC_LINK_PCI_EXPRESS, "PCIe",0,1);
				}
			}else if(g_module_state.ipmi_amc_host > AMC_FTRN_UNKNOWN_HOST){ //we got something from Libera or MCH
				// check if host type changed from previous check
				if(g_module_state.host_type != g_module_state.ipmi_amc_host){
					debug(3,"HOST_TYPE","Ipmi Msg: %X >> %X", g_module_state.host_type,g_module_state.ipmi_amc_host);
					g_module_state.host_type = g_module_state.ipmi_amc_host;

					// indicate with LED_WHITE to the user what host type was detected
					// blink time according to host type
					switch (g_module_state.ipmi_amc_host){
					case AMC_FTRN_IN_LIBERA_SLOT_7:
						info("HOST_TYPE","In Libera Slot7");
						on_delay  =  200;
						off_delay = 1800;

						iopin_set(PAYLOAD_PIN_IN_LIBERA);
						// turn off BLUE led in Libera because BCM does not send BLUE led control messages
						iopin_led(BLUE_LED,1,0);
						break;

					case AMC_FTRN_IN_LIBERA_SLOT_8:
						info("HOST_TYPE","In Libera Slot8");
						on_delay  = 1800;
						off_delay =  200;
						iopin_set(PAYLOAD_PIN_IN_LIBERA);

						// turn off BLUE led in Libera because BCM does not send BLUE led control messages
						iopin_led(BLUE_LED,1,0);
						break;
					case AMC_FTRN_IN_MICROTCA: // set when received SET_EVENT_RECEIVER from MCH
						
						// currently MTCA.4 triggers are enabled by SW via FPGA
						// MMC/MCH are not involved, MMC can only observe when SW enabled MTCA.4 triggers
						// by observing PAYLOAD_PIN_MTCA4_EN pin
						if(iopin_get(PAYLOAD_PIN_MTCA4_EN == 1)){
							// this will trigger transion to AMC_FTRN_IN_MICROTCA_4 host type
							g_module_state.ipmi_amc_host = AMC_FTRN_IN_MICROTCA_4;
						}
						// set when SET_EVENT_RECEIVER request was received from MTCA MCH
						// NOTE: Libera does not send this request therefore we know we are definetly not in Libera
						info("HOST_TYPE","MTCA");
						on_delay  = 1000;
						off_delay = 0;
						iopin_clear(PAYLOAD_PIN_IN_LIBERA);
						break;

					case AMC_FTRN_IN_MICROTCA_4: // if received SET_EVENT_RECEIVER request from MCH and MTCA.4 pin is HI
						if(iopin_get(PAYLOAD_PIN_MTCA4_EN == 0)){
							// this will trigger transion to AMC_FTRN_IN_MICROTCA host type
							g_module_state.ipmi_amc_host = AMC_FTRN_IN_MICROTCA;
						}
						info("HOST_TYPE","In MTCA.4");
						on_delay  = 1000;
						off_delay = 1000;
						iopin_clear(PAYLOAD_PIN_IN_LIBERA);
						break;

					default: // should not get here
						info("HOST_TYPE","Unknown");
						g_module_state.ipmi_amc_host = AMC_FTRN_UNKNOWN_HOST;
						on_delay  = 250;
						off_delay = 250;
						iopin_clear(PAYLOAD_PIN_IN_LIBERA);
						break;
					}
					
				}
			}else{
#ifdef LIBERA_HS_EVENT_HACK
				// we have power from backplane but we do not know yet
				// if we are in Libera or MTCA
				// default state AMC_FTRN_UNKNOWN_HOST
				if(g_module_state.host_type != AMC_FTRN_UNKNOWN_HOST){
					debug(3,"HOST_TYPE","Change: %X >> %X", g_module_state.host_type,AMC_FTRN_UNKNOWN_HOST);
					info("HOST_TYPE","Unknown");
          g_module_state.host_type     = AMC_FTRN_UNKNOWN_HOST;
					g_module_state.ipmi_amc_host = AMC_FTRN_UNKNOWN_HOST;
					on_delay  = 250;
					off_delay = 250;
					iopin_clear(PAYLOAD_PIN_IN_LIBERA);
				}
#else
				// // we have power from backplane therefore default state AMC_FTRN_IN_MICROTCA
				if(g_module_state.host_type != AMC_FTRN_IN_MICROTCA){
					debug(3,"HOST_TYPE","Change: %X >> %X", g_module_state.host_type,AMC_FTRN_IN_MICROTCA);
					info("HOST_TYPE","Dflt: In MTCA");
					on_delay  = 1000;
					off_delay = 0;
					iopin_clear(PAYLOAD_PIN_IN_LIBERA);
				}
#endif
			}
			
			
			// blink WHITE LED to indicate that FTRN is aware of the host type
			if(on_delay){
				iopin_led(LED_WHITE,0,1); //turn on LED
			}
			vTaskDelay(on_delay);

			iopin_led(LED_WHITE,0,0);
			vTaskDelay(off_delay);


		}else{
			g_module_state.host_type = AMC_FTRN_UNKNOWN_HOST;
			iopin_clear(PAYLOAD_PIN_IN_LIBERA);
			iopin_led(LED_WHITE,0,0);
			vTaskDelay(1000);
		}// payload active state
		
		
		
		
		
		
		

		// read character from serial port
		c=U0RBR;

		// check if user wanted something
		switch(c){
			case 'R':
				report_set = 1;
			    break;
			case 0x30 ... 0x39:
				c-=0x30;
				
				if (report_set){
					set_report_level(c); // change report level
					info("DEBUG","Report level set to %d",get_report_level());
					report_set = 0;
				}
			    else {
					//print out contents of the ws_array entry
					ws_print_content(c);
				}
				break;
			case 'i':
			case 'I':
				info("INFO","MMC build info:");
				printf(MMC_BUILD_ID);
				break;
			case 'p': // disable PCIe port
				info("INFO","Disabling PCIe port");
				payload_port_set_state(AMC_LINK_PCI_EXPRESS, "PCIe",0,0);
				break;
			case 'P': // enable PCIe port
				info("INFO","Enabling PCIe port");
				payload_port_set_state(AMC_LINK_PCI_EXPRESS, "PCIe",0,1);
				break;
			case 'l': // turn OFF all MMC LEDs
				info("INFO","LED test ON: LEDs OFF");
				g_module_state.led_test_mode = LED_TEST_ON_LED_OFF;
				iopin_led(LED_WHITE,0,0);
				iopin_led(LED_ERROR,0,0);
				iopin_led(LED_OK   ,0,0);
				iopin_led(BLUE_LED ,1,0);
				break;
			case 'L': // turn ON all MMC LEDs
				info("INFO","LED test ON: LEDs ON");
				g_module_state.led_test_mode = LED_TEST_ON_LED_ON;
				iopin_led(LED_WHITE,0,0);
				iopin_led(LED_ERROR,0,0);
				iopin_led(LED_OK   ,0,0);
				iopin_led(BLUE_LED ,1,0);
				break;
			case 'o': // turn OFF all LED test
				info("INFO","LED test OFF");
				g_module_state.led_test_mode = LED_TEST_OFF;
				break;
			case 't': // display taks list with status and stack info
				vTaskList(&statusmsg[0]);
				printf("\n--------------------------------------------------\n");
				printf("%s\n", statusmsg);
				break;
				
			case 'q': // check IPMI bus I2C pin state
				iopin_get(STANDALONE);
				iopin_get(STANDALONE);
				printf("\nIPMI I2C pin state\n");
				printf("PINSEL0= 0x%08X\n",PINSEL0);
				printf("IOPIN0 = 0x%08X\n",IOPIN0);
				printf("IODIR0 = 0x%08X\n\n",IODIR0);
				
				printf("I2STAT0=0x%08X\n",I2C0_I2STAT);
				printf("I2ADDR0=0x%08X\n",I2C_I2ADR);
				printf("I2DAT0 =0x%08X\n",I2C_I2DAT);
				printf("I2CONSET0=0x%08X\n\n", I2C0_I2CONSET);
				
				printf("I2C0SCLH=0x%08X\n", I2C0SCLH);
				printf("I2C0SCLL=0x%08X\n", I2C0SCLL);
				
				break;
				
			case 'D': // change IPMI bus I2C pins to GPIO inputs
				printf("\n\nIOPIN0=%08X state pre change\n",IOPIN0);
				printf("Changing IPMI bus I2C pins to GPIO inputs\n");
				iopin_dir_in(I2C_0_SCL);
				iopin_dir_in(I2C_0_SDA);
				PINSEL0 = PINSEL0 & 0xFFFFFF0F;
				printf("IOPIN0=%08X state post change\n",IOPIN0);
				
				break;
			case 'd': // change IPMI bus I2C pins to inputs
				printf("\nChanging IPMI bus I2C pins to I2C\n");
				PINSEL0 = PINSEL0 | 0x00000050;
				printf("IOPIN0=%08X state post change\n",IOPIN0);
				
				break;

			case 'e': // disabling I2C controller
				printf("\n\nIOPIN0=%08X state pre change\n",IOPIN0);
				printf("Disabling I2C controller\n");
				I2C0_I2CONCLR = 0x7F;
				printf("IOPIN0=%08X state post change\n",IOPIN0);
				
				break;
			case 'E': // Enabling I2C controller
				printf("\n\nIOPIN0=%08X state pre change\n",IOPIN0);
				printf("Enabling I2C controller in slave mode\n");
				I2C0_I2CONSET = 0x44;
				printf("IOPIN0=%08X state post change\n",IOPIN0);
				break;

			case 'r': // reset IPMI bus I2C controller
				printf("\nAsserting STOP on IPMI I2C bus\n\n");
				I2C0_I2CONSET = 0x10; // asserting STOP
				break;								

			case 'b': // set I2C0SCL frequency to 60k
               	I2C0SCLH = 100;
               	I2C0SCLL = 100;
				printf("\nI2C0 SCK frequency to 60kHz\n");
				break;

			case 'B': // set I2C0SCL frequency to 100k
               	I2C0SCLH = 60;
               	I2C0SCLL = 60;
                printf("\nI2C0 SCL frequency to 100kHz\n");
				break;

			case 'w': // print ws_array elements state
                printf("\nChecking ws_array element state:\n");
                ws_print_state(4);
				break;
				
			case 'u': 
                printf("\nUnclogin ws_array!\n");
                ws_unclog();
				break;

			case 's': // list last 256 I2C statuses
                printf("\nI2C Channel, status reg\n");
                list_i2c_status_log();
				break;

			case 'c': // print out CQ array
                cq_array_print();
				break;

            case 0x0A:
			case 0x0D: // add new line to console printout
                printf("\n");
				break;
			case 'h':
			case 'H':
				printf(HELP_TEXT);
		  default:
				break;
	/*
			}else if(c=='A'){ // Skip CONF_DONE check
				info("INFO","Forcing ACTIVE STATE");
				g_module_state.force_active = 1;
	*/


		}

	}//while(1)
}



static void init_task(void* pvt){
	info("INIT","Cosylab MMC startup!");
	info("INIT","CoreIPM init!");

	set_report_level(0); // no debug prints
	//CoreIPM init
	ws_init();
	i2c_initialize();
	timer_initialize();
	ipmi_initialize();
	picmg_init();
	module_init();

	info("INIT","Payload init!");
	payload_init();
	mmc_config_init();

  printf(MMC_BUILD_ID);

	// Create clock task
	xTaskCreate(tskCoreIPM_hardclock, "IPMCLK",configMINIMAL_STACK_SIZE,0,4,0);

	// Create Host type detect task - when powered up, detect in which type of host is FTRN
	xTaskCreate(tskHostType, "HOST_TYPE",4*configMINIMAL_STACK_SIZE,0,tskIDLE_PRIORITY,0);


	info("INIT","Starting coreIPM task");

	while(1){
		ws_process_work_list();
		timer_process_callout_queue();

		vTaskDelay(1);
	}
}


int main() {

  // configure IO pins according to schematic
	iopin_initialize();

  // configure communication modules
	uart_init();
	auxI2C_init();

	xTaskCreate(init_task,"INIT",configMINIMAL_STACK_SIZE*10,0,tskIDLE_PRIORITY+1,0);

	slowTask_sem = xSemaphoreCreateBinary();

	info("BOOT","Starting kernel");
	portENABLE_INTERRUPTS();
	info("BOOT","Starting scheduler");
	vTaskStartScheduler();
	error("BOOT","Scheduler stopped!\n");

	return 0;
}


/*
static void taskHello(void *pvParameters) {
	for (;;) {
		printf("Hello from task %d\n", ((int) pvParameters));

		vTaskDelay(1000);
	}

}


static void slowTask(void *pvt) {
	int delay = (int) pvt;
	while (1) {
		printf("[SLOW %d]: Begin processing\n", delay);
		int i = 0;
		for (i = 0; i < delay; i++) {
//			uncomment this if using cooperative scheduling
//			if(!(i%1000)) taskYIELD();
		}

		printf("[SLOW %d]: End processing\n", delay);
		xSemaphoreGive(slowTask_sem);
	}
}

static void waitTask(void* pvt){
	while(1){
		if(xSemaphoreTake(slowTask_sem,1000)){
			printf("Wait task woken up!\n");
		}
	}
}

*/
