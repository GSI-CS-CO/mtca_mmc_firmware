/*
-------------------------------------------------------------------------------
coreIPM/mmc.c

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
#include "../drivers/iopin.h"
#include "../drivers/mmcio.h"
#include "string.h"
#include "ipmi.h"
#include "amc.h"
#include "mmc.h"
#include "gpio.h"
#include "event.h"
#include "module.h"
//#include "lpc21nn.h"
//#include "i2c.h"
#include "event.h"
#include "debug.h"
//#include "arch.h"
#include "timer.h"
#include "ws.h"
#include "sensor.h"
//#include "mmcio.h"
#include "../payload.h"
#include "../util/report.h"

unsigned char mmc_ipmbl_address;
unsigned char mmc_state;
unsigned char mmc_hot_swap_state;
unsigned char switch_poll_timer_handle;
unsigned char module_monitor_voltage_handle;
unsigned send_event_retry_timer_handle = 0;
unsigned char pending_cmd_seq;

#define MMC_STATE_RESET		0
#define MMC_STATE_RUNNING	1

/* NOTES
The external interrupt function has four registers associated with it. The EXTINT
register contains the interrupt flags, and the EXTWAKEUP register contains bits 
that enable individual external interrupts to wake up the microcontroller from 
Power-down mode. The EXTMODE and EXTPOLAR registers specify the level and edge 
sensitivity parameters.

EXTINT The External Interrupt Flag Register contains interrupt flags for EINT0,
EINT1, EINT2 and EINT3.


Writing ones to bits EINT0 through EINT3 in EXTINT register clears the corresponding
bits.

INTWAKE The Interrupt Wakeup Register contains four enable bits that control
whether each external interrupt will cause the processor to wake up from Power-down
mode.

EXTMODE The External Interrupt Mode Register controls whether each pin is edge- or
level sensitive.

EXTPOLAR The External Interrupt Polarity Register controls which level or edge on
each pin will cause an interrupt. 
*/

#define EXTINT_FLAG_EINT0	1
#define EXTINT_FLAG_EINT1	2
#define EXTINT_FLAG_EINT2	4
#define EXTINT_FLAG_EINT3	8

AMC_PORT_STATE port_state;

#if defined (__CA__) || defined (__CC_ARM)
void EINT_ISR_0( void ) __irq;
#elif defined (__GNUC__)
void EINT_ISR_0(void) __attribute__ ((interrupt));
#endif

#if defined (__CA__) || defined (__CC_ARM)
void EINT_ISR_2( void ) __irq;
#elif defined (__GNUC__)
void EINT_ISR_2(void) __attribute__ ((interrupt));
#endif

unsigned char hot_swap_handle_last_state;

// FRU info data
extern unsigned char current_sensor_count;

extern SDR_ENTRY sdr_entry_table[];

/* Hot swap sensor records */
FULL_SENSOR_RECORD hssr;
SENSOR_DATA hssd;

void module_init2( void );
void mmc_hot_swap_state_change( unsigned char new_state );
void switch_state_poll( unsigned char *arg );
void send_event_retry( unsigned char *arg );
void fru_data_init( void );
void mmc_send_event_complete( void *ws, int status );
void hotswap_init_sensor_record( void );





/* MMC power - up

3.6.1 Typical Module insertion

When the Module�s Management Power is enabled, the BLUE LED turns on as soon 
as possible.

When the Module Handle in the Module is closed, the MMC sends a Module Hot Swap
(Module Handle Closed) event message to the Carrier IPMC, as described in 
Table�3-8, �Module Hot Swap event message.�

The Carrier IPMC sends a �Set FRU LED State� command to the MMC with a request
to perform long blinks of the BLUE LED, indicating to the operator that the new
Module is waiting to be activated.

The Carrier IPMC reads the Module�s Module Current Requirements record and 
AdvancedMC Point-to-Point Connectivity record.

REQ 3.75 The Module FRU Information shall contain the Module Current Requirements
record as shown in Table�3-10 Module Current Requirements record.
This is returned in the MultiRecord Area (see Ch16 in "- IPMI -
Platform Management FRU Information Storage Definition" document

The FRU Inventory data also may contain other information such as the serial
number, part number, asset tag, and short descriptive string for the FRU.
The contents of a FRU Inventory Record are also specified in the Platform Management
FRU Information Storage Definition.

If the Module FRU Information is invalid or if the Carrier cannot provide the
necessary Payload Power then:

The Carrier IPMC sends a �Set FRU LED State� command to the MMC requesting the
�on� state for the BLUE LED. The FRU remains in M1.

On receipt of the �Set FRU Activation (Activate FRU)� command by the Carrier 
IPMC, designating a particular Module, the Carrier IPMC sends an M2 to M3 
transition event message to the Shelf Manager on behalf of the Module and sends
a �set FRU LED State� command to the MMC with a request to turn off the BLUE LED.

The Carrier IPMC enables Payload Power (PWR) for the Module.
*/

/*==============================================================
 * INITIALIZATION
 *==============================================================*/
/*
 * module_init()
 *
 * Gets called when we power up.
 */
#define POLLED_HOT_SWAP
#ifdef POLLED_HOT_SWAP
void
module_init( void )
{
	unsigned char handle_state = iopin_get( HOT_SWAP_HANDLE );

	hot_swap_handle_last_state = handle_state;

	// ====================================================================
	/* Turn on blue LED. When the Module�s Management Power is enabled,
	 * the BLUE LED should turn on as soon as possible. */
	gpio_led_on( GPIO_FRU_LED_BLUE );

	mmc_state = MMC_STATE_RUNNING;
	i2c_interface_enable_local_control( 0, 0 );
    // fru_data_init();
	// hotswap_init_sensor_record();
	module_sensor_init();

	// ====================================================================
	// Handle current state of Hot Swap Handle 
	/*
	if( handle_state == HANDLE_SWITCH_CLOSED )
			mmc_hot_swap_state_change( MODULE_HANDLE_CLOSED );
	*/
	if ( handle_state == HANDLE_SWITCH_OPEN ) {
		mmc_hot_swap_state_change( MODULE_HANDLE_OPENED );
	} else {
		mmc_hot_swap_state_change( MODULE_HANDLE_CLOSED );
		payload_activate();
	}

	// ====================================================================
	// check the hot swap switch periodically
	timer_add_callout_queue( (void *)&switch_poll_timer_handle,
		       	5*HZ, switch_state_poll, 0 ); /* 5 sec timeout */

//	timer_add_callout_queue(&module_monitor_voltage_handle,HZ/10,module_monitor_dcVoltage,0);

}
#else
void
module_init( void )
{
	unsigned char reset_state = iopin_get( EINT_RESET );

	fru_data_init();
	//hotswap_init_sensor_record();
	module_sensor_init();
	module_payload_on();
	
	// level-sensitivity is selected for EINT2.
	EXTMODE &= 0xfb;	/* 0: Level-sensitive
			   1: edge sensitive. */ 

	/* select whether the corresponding pin is high- or low-active. */
	if( reset_state )  
		EXTPOLAR &= 0xfb;	/* 0: low-active or falling-edge sensitive
				   1: high-active or rising-edge sensitive */
	else
		EXTPOLAR |= 4;

	/* Whenever a change of external interrupt operating mode (i.e. active
	 * level/edge) is performed (including the initialization of an external
	 * interrupt), the corresponding bit in the EXTINT register must be cleared! */

	/* Writing ones to bits EINT0 through EINT3 in EXTINT register clears the 
	 * corresponding bits. */ 
	
	EXTINT = 0xff; // clear all bits
	
	VICVectAddr8 = ( unsigned long )EINT_ISR_2;	/* set interrupt vector in 7 */
	VICVectCntl8 = 0x20 | IS_EINT2;			/* use it for EINT2 interrupt */
	VICIntEnable = IER_EINT0;			/* enable EINT2 interrupt */

	if( !reset_state )	// a low indicates we're held in reset state
		return;
	
	module_init2();
}
#endif

void
module_init2( void )
{
	unsigned char handle_state;

	/* REQ 3.54 When Management Power (MP) is applied to the MMC and its
	associated management circuitry, the ENABLE# signal is active, and a 
	valid Geographic Address has been read, the MMC shall read the state of
	the Module Handle and send a Module Hot Swap (Module Handle Opened or 
	Module Handle Closed) event message appropriately, as described in 
	Table�3-8, �Module Hot Swap event message.� */
	
	// get our IPMB-L address
	mmc_ipmbl_address = module_get_i2c_address( I2C_ADDRESS_LOCAL );

	// module specific initialization for serial & i2c interfaces go in here

	/* initialize the Hot Swap Handle handler */

	// get current hot-swap-handle setting
	handle_state = iopin_get( EINT_HOT_SWAP_HANDLE );
	hot_swap_handle_last_state = handle_state;

	// level-sensitivity is selected for EINT0.
	EXTMODE &= 0xfe;	/* 0: Level-sensitive
			   1: edge sensitive. */ 

	/* select whether the corresponding pin is high- or low-active. */
	if( handle_state )  
		EXTPOLAR &= 0xfe;	/* 0: low-active or falling-edge sensitive
				   1: high-active or rising-edge sensitive */
	else
		EXTPOLAR |= 1;

	/* Whenever a change of external interrupt operating mode (i.e. active
	 * level/edge) is performed (including the initialization of an external
	 * interrupt), the corresponding bit in the EXTINT register must be cleared! */
	EXTINT = 0;
	
	VICVectAddr7 = ( unsigned long )EINT_ISR_0;	/* set interrupt vector in 7 */
	VICVectCntl7 = 0x20 | IS_EINT0;			/* use it for EINT0 interrupt */
//	VICIntEnable = IER_EINT0;			/* enable EINT0 interrupt */

	
	/* Turn on blue LED. When the Module�s Management Power is enabled,
	 * the BLUE LED should turn on as soon as possible. */
	gpio_led_on( GPIO_FRU_LED_BLUE );
	mmc_state = MMC_STATE_RUNNING;
	i2c_interface_enable_local_control( 0, 0 );

	// Handle current state of Hot Swap Handle 
	mmc_hot_swap_state_change( handle_state );



}

void
switch_state_poll( unsigned char *arg )
{
	unsigned char handle_state = iopin_get( HOT_SWAP_HANDLE );

	if( handle_state != hot_swap_handle_last_state ) {
		info("MMC","HotSwap handle %s\n", (handle_state == HANDLE_SWITCH_OPEN) ? "OPEN" : "CLOSED");
		if( handle_state == HANDLE_SWITCH_OPEN ){
			mmc_hot_swap_state_change( MODULE_HANDLE_OPENED );
		}else{
			mmc_hot_swap_state_change( MODULE_HANDLE_CLOSED );
			//handle closed, activate payload FSM
			payload_activate();
		}

		hot_swap_handle_last_state = handle_state;
	}
	
	// Re-start the timer
	timer_add_callout_queue( (void *)&switch_poll_timer_handle,
		       	1, switch_state_poll, 0 ); /* every 5hz */

}

void
module_rearm_events( void )
{
	unsigned char handle_state = iopin_get( HOT_SWAP_HANDLE );

	( handle_state == HANDLE_SWITCH_OPEN )?
			mmc_hot_swap_state_change( MODULE_HANDLE_OPENED ):
			mmc_hot_swap_state_change( MODULE_HANDLE_CLOSED );

}


//void
//hotswap_init_sensor_record( void )
//{
//	hssr.record_id[0] = 1;
//	hssr.sdr_version = 0x51;
//	hssr.record_type = 1;	/* Record Type Number = 01h, Full Sensor Record */
//	hssr.record_len = sizeof( FULL_SENSOR_RECORD ) - 5;	/* Number of remaining record bytes following. */
//	hssr.owner_id = 0;	/* 7-bit system software ID */
//	hssr.id_type = 1;	/* System software type */
//	hssr.channel_num = 0;
//	hssr.sensor_owner_lun = 0;
//	hssr.sensor_number = 0;	/* this will get replaced by the actual sensor number when we register the SDR */
//	hssr.entity_id = ENTITY_ID_SYSTEM_BOARD; /* physical entity the sensor is monitoring */
//
//	hssr.entity_type = 0;	/* treat entity as a physical entity */
//	hssr.entity_instance_num = 0;
//	hssr.init_scanning = 1;	/* the sensor accepts the �enable/disable scanning� bit in the
//				   Set Sensor Event Enable command). */
//	hssr.init_events = 0;
//	hssr.init_thresholds = 0;
//	hssr.init_hysteresis = 0;
//	hssr.init_sensor_type = 0;
//
//	/* Sensor Default (power up) State */
//	hssr.powerup_evt_generation = 0;	/* event generation disabled */
//	hssr.powerup_sensor_scanning = 1;	/* sensor scanning enabled */
//	hssr.ignore_sensor = 1;			/* Ignore sensor if entity is not present or disabled. */
//
//	/* Sensor Auto Re-arm Support */
//	hssr.sensor_manual_support = 1;		/* automatically rearms itself when the event clears */
//
//	/* Sensor Hysteresis Support */
//	hssr.sensor_hysteresis_support = 0; 	/* No hysteresis */
//
//	/* Sensor Threshold Access Support */
//	hssr.sensor_threshold_access = 0;	/* no thresholds */
//
//	/* Sensor Event Message Control Support */
//	hssr.event_msg_control = 1;			/* entire sensor only (implies that global
//						   disable is also supported) */
//
//	hssr.sensor_type = ST_HOT_SWAP;		/* From Table 42-3, Sensor Type Codes */
//	hssr.event_type_code = 0;		/* unspecified */
//	hssr.event_mask = 0;
//	hssr.deassertion_event_mask = 0;
//	hssr.reading_mask = 0;
//	hssr.analog_data_format = 0;		/* unsigned */
//	hssr.rate_unit = 0;			/* none */
//	hssr.modifier_unit = 0;			/* 00b = none */
//	hssr.percentage = 0;			/* not a percentage value */
//	hssr.sensor_units2 = SENSOR_UNIT_UNSPECIFIED;	/*  Base Unit */
//	hssr.sensor_units3 = 0;		/* no modifier unit */
//	hssr.linearization = 0;		/* Linear */
//	hssr.M = 0;
//	hssr.M_tolerance = 0;
//	hssr.B = 0;
//	hssr.B_accuracy = 0;
//	hssr.accuracy = 0;
//	hssr.R_B_exp = 0;
//	hssr.analog_characteristic_flags = 0;
//	hssr.nominal_reading;
//	hssr.normal_maximum = 0;
//	hssr.normal_minimum;
//	hssr.sensor_maximum_reading = 0xff;
//	hssr.sensor_minimum_reading = 0;
//	hssr.upper_non_recoverable_threshold = 0;
//	hssr.upper_critical_threshold = 0;
//	hssr.upper_non_critical_threshold = 0;
//	hssr.lower_non_recoverable_threshold = 0;
//	hssr.lower_critical_threshold = 0;
//	hssr.lower_non_critical_threshold;
//	hssr.positive_going_threshold_hysteresis_value;
//	hssr.negative_going_threshold_hysteresis_value;
//	hssr.reserved2 = 0;
//	hssr.reserved3 = 0;
//	hssr.oem = 0;
//	hssr.id_string_type = 3;	/* 11 = 8-bit ASCII + Latin 1. */
//	hssr.id_string_length = 11; /* length of following data, in characters */
//	memcpy( hssr.id_string_bytes, "HOT SWAP Sensor", 15 ); /* Sensor ID String bytes. */
//
////	 sensor_add( &hssr, &hssd );
//}


/*==============================================================
 * STATE CHANGE HANDLING
 *==============================================================*/
/*
REQ 3.55 When the Module Handle transitions to closed, the MMC shall 
send a Module Hot Swap (Module Handle Closed) event message as described
in Table�3-8, �Module Hot Swap event message.�

REQ 3.56 When the Module Handle state transitions to open, the MMC shall
send a Module Hot Swap (Module Handle Opened) event message as described
in Table�3-8, �Module Hot Swap event message.�

REQ 3.57 MMCs shall periodically re-send Module Hot Swap event messages 
until either a Command Completed Normally (00h) Completion Code has been 
returned from the Carrier IPMC or the Module wants to send a new Module 
Hot Swap event message.


*/
/* Arguments;
 * 	new_state =   [ MODULE_HANDLE_CLOSED | 
 * 			MODULE_HANDLE_OPENED |
 * 			MODULE_QUIESCED |
 * 			MODULE_BACKEND_POWER_FAILURE |
 * 			MODULE_BACKEND_POWER_SHUTDOWN ]
 */
void
mmc_hot_swap_state_change( unsigned char new_state )
{
	info("MMC","HS State Change: %d\n",new_state);
	FRU_HOT_SWAP_EVENT_MSG_REQ msg_req;

	/* When the Module Handle state in the Module is changed, the MMC sends a 
	 * Module Hot Swap (Module Handle Closed) event message to the Carrier IPMC, 
	 * as described in Table�3-8, �Module Hot Swap event message.� */

	/*
	Each MMC contains one Module Hot Swap sensor. This sensor proactively generates events
	(Module Handle Closed, Module Handle Opened, Quiesced, Backend Power Shut Down,
	and Backend Power Failure) to enable the Carrier IPMC to perform Hot Swap management
	for the Modules it represents.
	*/

	msg_req.command = IPMI_SE_PLATFORM_EVENT;
	msg_req.evt_msg_rev = IPMI_EVENT_MESSAGE_REVISION;
	msg_req.sensor_type = IPMI_SENSOR_MODULE_HOT_SWAP;
	msg_req.sensor_number = 1;		/* Hot swap sensor number */
	msg_req.evt_direction = IPMI_EVENT_TYPE_GENERIC_AVAILABILITY;
	msg_req.evt_data1 = new_state;	
	msg_req.evt_data2 = 0xff;	
	msg_req.evt_data3 = 0xff;	

	// remove any previously undelivered events from the callout queue
	timer_remove_callout_queue( (void *)&send_event_retry_timer_handle );
	if( send_event_retry_timer_handle )	 {
		ws_free( ( IPMI_WS * )send_event_retry_timer_handle );
		send_event_retry_timer_handle = 0;
	}

	/* dispatch message */
	pending_cmd_seq = ipmi_send_event_req( ( unsigned char * )&msg_req, sizeof( FRU_HOT_SWAP_EVENT_MSG_REQ ), mmc_send_event_complete );
}


void
mmc_send_event_complete( void *ws, int status )
{
	switch ( status ) {
		case XPORT_REQ_NOERR:
			ws_free( (IPMI_WS *)ws );
			break;
		case XPORT_REQ_ERR:
		case XPORT_RESP_NOERR:
		case XPORT_RESP_ERR:
		default:
			// If event delivery failed, start the timer and retry
			send_event_retry_timer_handle = ( unsigned int )ws;
 			timer_add_callout_queue( (void *)&send_event_retry_timer_handle,
		       		5*HZ, send_event_retry,( unsigned char * )ws ); /* 5 sec timeout */

			break;
	}
}


void
send_event_retry( unsigned char *arg )
{
	IPMI_WS *ws = ( IPMI_WS * )arg;

	ws_set_state( ws, WS_ACTIVE_MASTER_WRITE );
}


void
module_process_response( 
	IPMI_WS *req_ws, 
	unsigned char seq,
	unsigned char completion_code )
{
	switch( completion_code ) {
		case CC_NORMAL:
		case CC_BUSY:
		case CC_INVALID_CMD:
		case CC_INVALID_CMD_FOR_LUN:
		case CC_TIMEOUT:
		case CC_OUT_OF_SPACE:
		case CC_RESERVATION:
		case CC_RQST_DATA_TRUNCATED:
		case CC_RQST_DATA_LEN_INVALID:
		case CC_DATA_LEN_EXCEEDED:
		case CC_PARAM_OUT_OF_RANGE:
		case CC_CANT_RETURN_REQ_BYTES:
		case CC_REQ_DATA_NOT_AVAIL:
		case CC_INVALID_DATA_IN_REQ:
		case CC_CMD_ILLEGAL:
		case CC_CMD_RESP_NOT_PROVIDED:
		case CC_CANT_EXECUTE_DUP_REQ:
		case CC_SDR_IN_UPDATE_MODE:
		case CC_FW_IN_UPDATE_MODE:
		case CC_INITIALIZATION:
		case CC_DEST_UNAVAILABLE:
		case CC_SECURITY_RESTRICTION:
		case CC_NOT_SUPPORTED:
		case CC_PARAM_ILLEGAL:
		case CC_UNSPECIFIED_ERROR:
		default:
			debug(2, "MMC","Process Response CC 0x%02X", completion_code);
			break;
	}

}


void
mmc_reset_state( void )
{
	mmc_state = MMC_STATE_RESET;

	// disable the i2c interface
	i2c_interface_disable( 0, 0 );
}

/*==============================================================
 * INTERRUPT SERVICE ROUTINES
 *==============================================================*/
//
///* This ISR is called when reset line state changes */
///*
//REQ 3.38 If the watchdog timer or the negation of ENABLE# resets the MMC,
//the Payload state shall not be impacted.
//*/
//#if defined (__CA__) || defined (__CC_ARM)
//void EINT_ISR_2( void ) __irq
//#elif defined (__GNUC__)
//void EINT_ISR_2( void )
//#endif
//{
//	unsigned char reset_state;
//
//	if( !( EXTINT & EXTINT_FLAG_EINT2 ) ) { // if not one of ours
//		VICVectAddr = 0;       		 // Acknowledge Interrupt & return
//		return;
//	}
//
//	reset_state = iopin_get( EINT_RESET );
//
//	if( reset_state ) {	// reset line  de-asserted
//		module_init2();
//	} else {		// reset line asserted
//		mmc_reset_state();
//	}
//
//	// setup for the next state change
//	if( reset_state )
//		EXTPOLAR &= 0xfb;
//	else
//		EXTPOLAR |= 4;
//
//	EXTINT = EXTINT_FLAG_EINT2;		// clear bit
//
//	VICVectAddr = 0;          		// Acknowledge Interrupt
//}


/*
3.6.2 Typical Module extraction

The operator can initiate deactivation by pulling the Module Handle, which 
changes the state of the Module Handle to open. When the Module Handle in
the Module is opened, the MMC in the Module sends a Module Hot Swap (Handle 
Opened) event message to the Carrier IPMC (as described in Section�3-8, 
�Module Hot Swap event message.� )

The IPMC in the Carrier sends a �Set FRU LED State� command to the MMC 
in the Module with a request to perform short blinks of the BLUE LED. 
This indicates to the operator that the Module is waiting to be deactivated.

The Carrier IPMC now issues �Set AMC Port State (Disable)� command(s) 
for all Ports on the Module and for all Ports that connect to the Module. 
This will disable all Ports associated with the Module about to be removed.

When the Carrier IPMC has transitioned the Module to M6 state, the Carrier 
IPMC sends a �FRU Control (Quiesce)� command to the Module and awaits a 
Module Hot Swap (Quiesced) event message from the MMC.

Next, the Carrier IPMC disables the Module�s Payload Power.
the Carrier IPMC sends a �Set FRU LED State� command to the MMC with a 
request to turn on the BLUE LED. This indicates to the operator that 
the Module is ready to be safely extracted.

The operator removes the Module.
*/



/*==============================================================
 * MMC SPECIFIC COMMANDS
 *==============================================================*/
/*
Module MMC req. mandatory commands

IPMI commands
-------------
Get Device ID
Broadcast �Get Device ID�[1]
Set Event Receiver
Get Event Receiver
Platform Event (a.k.a. �Event Message�)
Get Device SDR Info
Get Device SDR
Reserve Device SDR Repository
Get Sensor Reading
Get FRU Inventory Area Info
Read FRU Data
Write FRU Data

PICMG related commands
----------------------
Get PICMG Properties
FRU Control
Get FRU LED Properties
Get LED Color Capabilities
Set FRU LED State
Get FRU LED State
Get Device Locator Record ID
FRU Control Capabilities
Set AMC Port State
Get AMC Port State
Set Clock State
Get Clock State
*/
/*
3.9.1.4 Set AMC Port State command
The �Set AMC Port State� command is used to enable or disable Ports associated with an
AdvancedMC Channel using Link Descriptor information from an AdvancedMC point-topoint
connectivity record. The AMC specification does not mandate that drivers for LVDS
Ports have the physical ability to be disabled or enabled. The �Set AMC Port State�
command contains Link information. The Link information could be used by Modules that
provide a configurable interface. The Module could configure the interface to match the type
used in the �Set AMC Port State� command allowing for the creation of Modules that can be
configured to support a variety of Fabric Interfaces. The MMC might receive this command
at any time as other Modules are inserted into or extracted from the Carrier.


3.9.1.5 Get AMC Port State command
The �Get AMC Port State� command provides a way to query the current Link, if any, on an
AdvancedMC Channel. In the request, the AdvancedMC Channel ID is provided to the
MMC. The MMC returns a response containing the state of that Link.

3.9.2 Clock E-Keying
By default, all AMC clocks and on-Carrier clock resources are in the 
disabled state when the AMC Module or Carrier is initially activated.

Requirements
REQ 3.182 Carriers and Modules shall support the �Set Clock State� command
defined in Table�3-44, � Set Clock State command� for all implemented clocks.

REQ 3.186 Carriers and Modules shall support the �Get Clock State� command 
defined in Table�3-45, � Get Clock State command.� If the designated clock 
resource has no clock enabled, Carriers and Modules shall indicate the clock 
as �Disabled� in the Clock State field.

3.10 Module Payload control
The �FRU Control Capabilities� Command provides a way to query which 
specific options an AdvancedMC supports in the �FRU Control� command. These
capabilities are expected to be static throughout the life of the AdvancedMC.
The �FRU Control (Cold Reset)� variant is mandatory, so the corresponding 
bit is marked reserved and the System Manager can assume that it is always 
supported.

REQ 3.53 The �Get Sensor Reading� command shall be implemented in the 
MMC to enable the Carrier IPMC to determine the current state of the 
Module. The Carrier IPMC can issue this command at any time to get 
the sensor status. This command can also be used by a Carrier when it
regains contact with a Module after a loss of contact (see Section�
3.6.8, �Communication lost.� )
*/

char *
link_type_name(uchar link_type)
{
	switch (link_type) {
	case AMC_LINK_PCI_EXPRESS:
		return "PCIe";
		break;
	case AMC_LINK_PCI_EXPRESS_AS:
		return "PCIe AS";
		break;
	case AMC_LINK_PCI_EXPRESS_AS2:
		return "PCIe AS2";
		break;
	case AMC_LINK_ETHERNET:
		return "Ethernet";
		break;
	case AMC_LINK_SERIAL_RAPID_IO:
		return "SRIO";
		break;
	case AMC_LINK_STORAGE:
		return "Storage";
		break;
	default:
		return "Reserved";
		break;
	}
	return "";

}  /* link_type_name */

/* 
 * mmc_process_command()
 *
 * Handle MMC specific commands
 */
void
mmc_set_port_state( IPMI_PKT *pkt )
{
	SET_AMC_PORT_STATE_CMD_REQ	*req = ( SET_AMC_PORT_STATE_CMD_REQ * )pkt->req;
	SET_AMC_PORT_STATE_CMD_RESP	*resp = ( SET_AMC_PORT_STATE_CMD_RESP * )pkt->resp;

	info("MMC", "Setting port state:%d ch:%d",
		 req->state,
		 req->amc_channel_id
	);
	info("MMC", "lane3210:%d%d%d%d",
		 req->lane_3_bit_flag,
		 req->lane_2_bit_flag,
		 req->lane_1_bit_flag,
		 req->lane_0_bit_flag
	);

	info("MMC", "link type:%d(%s)", req->link_type, link_type_name(req->link_type));
	info("MMC",	"link type ext:%d", req->link_type_ext);
	info("MMC", "link group ID:%d\n", req->link_grp_id );


	resp->completion_code = CC_NORMAL;
	resp->picmg_id = PICMG_ID;
	pkt->hdr.resp_data_len = sizeof( SET_AMC_PORT_STATE_CMD_RESP ) - 1;

	port_state.link_grp_id     = req->link_grp_id;
	port_state.link_type_ext   = req->link_type_ext;
	port_state.link_type       = req->link_type;
	port_state.lane_3_bit_flag = req->lane_3_bit_flag;
	port_state.lane_2_bit_flag = req->lane_2_bit_flag;
	port_state.lane_1_bit_flag = req->lane_1_bit_flag;
	port_state.lane_0_bit_flag = req->lane_0_bit_flag;
	port_state.amc_channel_id  = req->amc_channel_id;
	port_state.state           = req->state;
//	port_state.on_carrier_dev_id = req->on_carrier_dev_id;

	payload_port_set_state(port_state.link_type,link_type_name(port_state.link_type),
			               port_state.amc_channel_id,req->state);
}

void
mmc_get_port_state( IPMI_PKT *pkt )
{
	//GET_AMC_PORT_STATE_CMD_REQ	*req  = ( GET_AMC_PORT_STATE_CMD_REQ * )pkt->req;
	GET_AMC_PORT_STATE_CMD_RESP	*resp = ( GET_AMC_PORT_STATE_CMD_RESP * )pkt->resp;

	info("MMC", "Get port state" );

	resp->completion_code = CC_NORMAL;
	resp->picmg_id = PICMG_ID;
	
	// return port state info if available - we don't have anything to report right now
	pkt->hdr.resp_data_len = 1;	
}

void
mmc_set_clock_state( IPMI_PKT *pkt )
{
	//SET_CLOCK_STATE_CMD_REQ	*req   = ( SET_CLOCK_STATE_CMD_REQ * )pkt->req;
	SET_CLOCK_STATE_CMD_RESP *resp = ( SET_CLOCK_STATE_CMD_RESP * )pkt->resp;

	info("MMC","Set clock state" );

	resp->completion_code = CC_NORMAL;
	resp->picmg_id = PICMG_ID;
	pkt->hdr.resp_data_len = sizeof( SET_CLOCK_STATE_CMD_RESP ) - 1;
}

void
mmc_get_clock_state( IPMI_PKT *pkt )
{
	//GET_CLOCK_STATE_CMD_REQ	*req   = ( GET_CLOCK_STATE_CMD_REQ * )pkt->req;
	GET_CLOCK_STATE_CMD_RESP *resp = ( GET_CLOCK_STATE_CMD_RESP * )pkt->resp;

	info("MMC", "Get clock state" );

	resp->completion_code = CC_NORMAL;
	resp->picmg_id = PICMG_ID;
	pkt->hdr.resp_data_len = sizeof( GET_CLOCK_STATE_CMD_RESP ) - 1;
}

void
mmc_get_fru_control_capabilities( IPMI_PKT *pkt ) 
{
	//FRU_CONTROL_CAPABILITIES_CMD_REQ *req = ( FRU_CONTROL_CAPABILITIES_CMD_REQ * )pkt->req;
	FRU_CONTROL_CAPABILITIES_CMD_RESP *resp = ( FRU_CONTROL_CAPABILITIES_CMD_RESP * )pkt->resp;

	info("MMC", "Get FRU control capabilities" );

	resp->completion_code = CC_NORMAL;
	resp->picmg_id = PICMG_ID;
	resp->diag_int = 1;		/* Capable of issuing a diagnostic interrupt */
	resp->graceful_reboot = 1;	/* Capable of issuing a graceful reboot */
	resp->warm_reset = 1;		/* Capable of issuing a warm reset */

	pkt->hdr.resp_data_len = sizeof( FRU_CONTROL_CAPABILITIES_CMD_RESP ) - 1;
}







/*==============================================================
 * MODULE SENSORS
 *==============================================================*/

/*
== HOT SWAP SENSOR ==
3.6.6 Module Hot Swap sensor
 46 Each MMC contains one Module Hot Swap sensor. This sensor proactively generate events
(Module Handle Closed, Module Handle Opened, Quiesced, Backend Power Shut Down,
and Backend Power Failure) to enable the Carrier IPMC to perform Hot Swap management
for the Modules it represents.

A Module�s Backend Power includes all the power supplies on a Module derived from
Payload Power. Module Backend Power could be disabled by a Module as an
implementation defined option. How a Module Payload communicates with the MMC to
indicate it has Quiesced, has requested that its Module Backend Power be shut down, or to
indicate a Module Backend Power Failure is implementation defined.

REQ 3.52 A Module Hot Swap sensor shall be implemented in the MMC.

REQ 3.158 MMCs on Modules implementing local on/ off switching control of 
power derived from Payload Power shall set the Backend Power Shut Down bit
in the Module Hot Swap sensor Current State Mask field to 1b when the MMC
has turned off its Module Backend Power. MMCs on Modules implementing local
on/ off switching control of power derived from Payload Power shall clear 
the Backend Power Shut Down bit to 0b when Payload Power from the Carrier 
transitions from disabled to enabled.

REQ 3.159 MMCs on Modules implementing local on/ off switching control 
of power derived from Payload Power shall set the Backend Power Failure
bit in the Module Hot Swap sensor Current State Mask field to 1b when
the Module Backend Power fails. MMCs on Modules implementing local on/ 
off switching control of power derived from Payload Power shall clear 
the Backend Power Failure bit to 0b when Payload Power from the Carrier
transitions from disabled to enabled.

REQ 3.160 MMCs shall set the Quiesced bit in the Module Hot Swap sensor 
Current State Mask field to 1b after the Module Payload has quiesced. 
The MMC shall clear the Quiesced bit to 0b upon reception of the �FRU
Control (Quiesce)� request just prior to  the Module Payload being quiesced. 
MMCs with Payload Power monitoring capability should clear the Quiesced 
bit to 0b when Payload Power from the Carrier transitions from disabled
to enabled.

== TEMPERATURE SENSORS ==
3.8 Cooling management
To support higher level management in appropriately managing the cooling resources, the
Module must provide reports of abnormal temperature in its environment. Every Module
must have temperature sensors; when an MMC detects that a monitored temperature sensor
crosses one or more thresholds in either direction, the MMC sends an IPMI temperature
event message to the Carrier IPMC. The Carrier IPMC, or higher level management, uses
this information to manage the cooling.

Every Carrier and every Module must contain at least two temperature sensors and
appropriate Sensor Data Records (SDRs) to describe the sensors. See Section 5.3,
�Temperature� for functional requirements regarding these sensors.

Requirements
REQ 3.164 The Module and the Carrier shall implement the SDRs for the 
temperature sensors required by Section�5, �Thermal.� 

REQ 3.80 The Module shall generate temperature sensor events in accordance 
with Section 3.9.3.2 �Abnormal Event Message� of the PICMG 3.0 specification.

REQ 3.165For each on-Module temperature sensor, Modules shall provide the 
warning operating temperature (upper non critical threshold) and the maximum
operating temperature (Upper critical threshold) in the SDR information.



3.11.1 Module SDR requirements

REQ 3.106 The MMC shall support IPMI commands �Get Device SDR Info�, �Get Device SDR� 
and �Reserve Device SDR Repository�. (See Chapter 29 of the IPMI specification.)

REQ 3.107 The MMC Device SDR shall have a static sensor population. (See 
Table 29-2 of the IPMI specification.) The number of MMC SDRs is fixed. 
All MMC sensors shall be assigned to LUN 0.

REQ 3.108 The MMC shall use AdvancedMC Module entity ID C1h for all device SDRs.

REQ 3.109 The MMC shall use its own Site Number + 60h as a device-relative
entity instance number. 

REQ 3.110Each MMC shall have one management controller device locator (SDR type 12h).

REQ 3.111 The MMC shall not have FRU locator records (SDR type 11h) as only
one FRU may be represented by one MMC.

REQ 3.112 The MMC shall not expect any Init Agent action from the Carrier IPMC.

REQ 3.113 The MMC shall initialize its sensors, set the event receiver address 
to 20h and event receiver LUN to 0 on reset.

3.12 FRU Information

All Carriers and Modules must contain a FRU Information storage device 
(for instance, an SEEPROM) that contains information about capabilities 
(e.g. E-Keying) and inventory data. The format of the FRU Information follows
the requirements set forth in Section 3.6.3, IPM Controller FRU Information,
in the PICMG 3.0 Specification. In addition to this basic information, 
additional records are required to support functions unique to the Modules.

3.12.1.2 MMC requirements

REQ 3.138 The MMC shall support the �FRU Inventory Device� commands specified
in Chapter 28 of the IPMI specification.

REQ 3.139Module FRU Information shall be available when MP is available.

REQ 3.140The entity updating the Module FRU Information shall be responsible 
for updating all appropriate checksums as well.

REQ 3.141 The MMC may write protect data in the Module storage area and 
return a Completion Code of �write protected offset (80h)� for a command
that attempts to change such data.
	
REQ 3.142If an MMC implements write protected data, it shall do it by areas
(Board Info, Product Info, etc.) with the exception of the multi-record area
where it shall implement write protection at the record level.

REQ 3.143 If an MMC implements write protected multi-record data, it shall
allow moving a record to a new offset (without any change to the data). That
is, the size or number of records prior to a particular record may change 
and it may be necessary for system management software to move a record 
without changing its contents. The MMC shall allow this, but may confirm
that the data has not been changed.

REQ 3.144 MMC FRU Information shall meet the requirements in Chapter 3.6.3
of the IPMI specification.REQ 3.145The MMC shall implement the relevant 
records defined in Section�3.7, �Power management� and Section�3.9, �E-Keying.� 

REQ 3.146 All Modules that are sold as independent products shall fill
in all predefined fields in Product Info Area record, as this is the 
only location where product version number is defined.

REQ 3.147Every MMC shall place the MultiRecords defined by this specification
in its MultiRecord Info Area.

REQ 3.148Any MMC may place private data in the Internal Use Area and/or MultiRecord Info Area.
*/



