/*
-------------------------------------------------------------------------------
coreIPM/mmcio.c

Author: Gokhan Sozmen
-------------------------------------------------------------------------------
Copyright (C) 2007-2008 Gokhan Sozmen
-------------------------------------------------------------------------------
coreIPM is free software; you can redistribute it and/or modify it under the 
terms of the GNU General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later 
version.

coreIPM is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with 
coreIPM; if not, write to the Free Software Foundation, Inc., 51 Franklin
Street, Fifth Floor, Boston, MA 02110-1301, USA.
-------------------------------------------------------------------------------
See http://www.coreipm.com for documentation, latest information, licensing, 
support and contact details.
-------------------------------------------------------------------------------
*/

#include <stdio.h>
#include "arch.h"
#include "../coreIPM/timer.h"
//#include "../coreIpm/debug.h"
#include "../coreIPM/ipmi.h"
#include "lpc21nn.h"
#include "mmcio.h"
#include "iopin.h"
#include "../coreIPM/module.h"
#include "../coreIPM/gpio.h"
#include "../coreIPM/event.h"
#include "../coreIPM/ws.h"
#include "../coreIPM/mmc.h"

void sensor_send_event(void);


/**
 * Set desired led ON
 */
void
module_led_on( unsigned led_state )
{
	long long iopin = 0;

	if( led_state & GPIO_LED_0 ) iopin |= LED_0;
	if( led_state & GPIO_LED_1 ) iopin |= LED_1;
	iopin_set( iopin );

}

void
module_led_off( unsigned led_state )
{
	long long iopin = 0;

	if( ~led_state & GPIO_LED_0 ) iopin |= LED_0;
	if( ~led_state & GPIO_LED_1 ) iopin |= LED_1;

	iopin_clear( iopin );
}

char payload_wait_pg_handle;
void payload_power_wait_pg(){
	printf("INFO: Waiting for power good\n");

	if(iopin_get(PAYLOAD_PIN_POWER_GOOD)){
		printf("INFO: powergood asserted, releasing payload reset\n");

		//remove from timer
		timer_remove_callout_queue(&payload_wait_pg_handle);

		//TODO: add payload reset release
	}else{
		printf("ERR: Powergood not asserted!\n");
		sensor_send_event();
		mmc_hot_swap_state_change(MODULE_BACKEND_POWER_FAILURE);
		timer_add_callout_queue(&payload_wait_pg_handle,HZ/10,payload_power_wait_pg,0);
	}
}

void
send_event_complete( void *ws, int status )
{
	switch ( status ) {
		case XPORT_REQ_NOERR:
			printf("DBG: Sensor data sent ok!\n");
			ws_free( (IPMI_WS *) ws );
			break;
		case XPORT_REQ_ERR:
		case XPORT_RESP_NOERR:
		case XPORT_RESP_ERR:
		default:
			printf("ERR: Could not send event!\n");
			break;
	}
}

void sensor_send_event(void){
	FRU_HOT_SWAP_EVENT_MSG_REQ msg_req;
	msg_req.command = IPMI_SE_PLATFORM_EVENT;
	msg_req.evt_msg_rev = IPMI_EVENT_MESSAGE_REVISION;
	msg_req.sensor_type = 0x02;
	msg_req.sensor_number = 0x02;		/* Hot swap sensor is 0 */
	msg_req.evt_direction = IPMI_EVENT_TYPE_GENERIC_AVAILABILITY;
	msg_req.evt_data1 = 0xab;
	msg_req.evt_data2 = 0xff;
	msg_req.evt_data3 = 0xff;

	/* dispatch message */
	printf("DBG: Sending sensor event!\n");
	ipmi_send_event_req( ( unsigned char * )&msg_req, sizeof( FRU_HOT_SWAP_EVENT_MSG_REQ ), send_event_complete );

}

/**
 * Enable PP_DC_EN and wait predefined amount of time for power good, signal.
 * If power good signal is not asserted in time, report an error.
 */
void
module_payload_on( void )
{
	//return if payload power is already enabled
//	if(g_module.module_payload_enabled) return;
//
//	printf("INFO: Payload ON\n");
//
//	iopin_set( PAYLOAD_PIN_POWER );
//	g_module.module_payload_enabled=1;
//	payload_power_wait_pg();
}

void
module_payload_off( void )
{
	//Return if payload power is already disabled
//	if(!g_module.module_payload_enabled) return;
//
//	//Remove waiting for pg from queue
//	timer_remove_callout_queue(&payload_wait_pg_handle);
//
//	printf("INFO: Payload OFF\n");
//	g_module.module_payload_enabled=0;
//	iopin_clear( PAYLOAD_PIN_POWER );
}



