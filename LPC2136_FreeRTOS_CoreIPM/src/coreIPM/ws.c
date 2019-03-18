/*
-------------------------------------------------------------------------------
coreIPM/ws.c

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

#include "../drivers/arch.h"
#include "ipmi.h"
#include "i2c.h"
#include "debug.h"
//#include "serial.h"
#include "ws.h"
#include "../critical.h"
#include "../payload.h"

#include <stdio.h>

extern unsigned long lbolt;

/*======================================================================*
 * WORKING SET MANAGEMENT
 */
IPMI_WS	ws_array[WS_ARRAY_SIZE];


/* initialize ws structures */
void 
ws_init( void )
{
	unsigned i;
	
	CRITICAL_START
	for ( i = 0; i < WS_ARRAY_SIZE; i++ )
	{
		ws_array[i].ws_state = WS_FREE;
	}
	CRITICAL_END
}

/* get a free ws elem */
IPMI_WS *
ws_alloc( void )
{
	IPMI_WS *ws = 0;
	IPMI_WS *ptr = ws_array;
	unsigned i;

	CRITICAL_START
	for ( i = 0; i < WS_ARRAY_SIZE; i++ )
	{
		ptr = &ws_array[i];
		if( ptr->ws_state == WS_FREE ) {
			ptr->ws_state = WS_PENDING;
			ws = ptr;
			break;
		}
	}
	CRITICAL_END
	return ws;
}

/* set ws state to free */
void 
ws_free( IPMI_WS *ws )
{
	int len, i;
	char *ptr = (char *)ws;

	CRITICAL_START
	len = sizeof( IPMI_WS );
	for( i = 0 ; i < len ; i++ ) {
		*ptr++ = 0;
	}	
	ws->incoming_protocol = IPMI_CH_PROTOCOL_NONE;
	ws->ws_state = WS_FREE;
	CRITICAL_END
}

IPMI_WS *
ws_get_elem( unsigned state )
{
	IPMI_WS *ws = 0;
	IPMI_WS *ptr = ws_array;
	unsigned i;

	CRITICAL_START
	for ( i = 0; i < WS_ARRAY_SIZE; i++ )
	{
		ptr = &ws_array[i];
		if( ptr->ws_state == state ) {
			if( ws ) {
				if( ptr->timestamp < ws->timestamp ) 
					ws = ptr;
			} else {
				ws = ptr;
			}
		}
	}
	
	if( ws )
		ws->timestamp = lbolt;
	CRITICAL_END
	return ws;
}

IPMI_WS *
ws_get_elem_seq( uchar seq, IPMI_WS *ws_ignore )
{
	IPMI_WS *ws = 0;
	IPMI_WS *ptr = ws_array;
	unsigned i;

	CRITICAL_START
	for ( i = 0; i < WS_ARRAY_SIZE; i++ )
	{
		ptr = &ws_array[i];
		if( ptr == ws_ignore )
			continue;
		if( ( ptr->ws_state != WS_FREE ) && ( ptr->seq_out == seq ) ) {
			ws = ptr;
			break;
		}
	}
	CRITICAL_END
	return ws;
}

void
ws_unclog() {
        int i;
        for (i = 0; i < WS_ARRAY_SIZE; i++) {
                ws_free(&ws_array[i]);
        }
}

void
ws_set_state( IPMI_WS * ws, unsigned state )
{
  if (state >= WS_FREE && state <= WS_ACTIVE_MASTER_READ_PENDING){
	  CRITICAL_START
	    ws->ws_state = state;
    CRITICAL_END
  }
}

/*==============================================================
 * ws_process_work_list()
 * 	Go through the active list, calling the ipmi handler for 
 * 	incoming entries and transport handler for outgoing entries.
 *==============================================================*/
void ws_process_work_list( void ) 
{
	IPMI_WS *ws;
	
	int hs_event = 0;
	int in_libera = 0;

	ws = ws_get_elem( WS_ACTIVE_IN );
	if( ws ) {
		ws_set_state( ws, WS_ACTIVE_IN_PENDING );
		// ipmi_process_pkt( ws );
		if( ws->ipmi_completion_function )
			( ws->ipmi_completion_function )( (void *)ws, XPORT_REQ_NOERR );
		else
			ws_process_incoming( ws ); // otherwise call the default handler
	}

	ws = ws_get_elem( WS_ACTIVE_MASTER_WRITE );
	if( ws ) {  

#ifdef LIBERA_HS_EVENT_HACK
		// HACK: since Libera ICB/BMC works only as I2C master on IPMI therefore
		// it expects from MMC only responses to request and can not process events
		// sent by MMC itself (like sensor events)
		// Therefore we need to check for HotSwap event.
		
		// check if outgoing packet is event
		hs_event =	(ws->pkt_out[4] == IPMI_SE_PLATFORM_EVENT        )
					// && (ws->pkt_out[5] == IPMI_EVENT_MESSAGE_REVISION   )
					 && (ws->pkt_out[6] == IPMI_SENSOR_MODULE_HOT_SWAP   )
					// && (ws->pkt_out[8] == IPMI_EVENT_TYPE_GENERIC_AVAILABILITY)
					;
		// check if we are in Libera or do not know yet
		in_libera = (g_module_state.host_type == AMC_FTRN_IN_LIBERA_SLOT_7) || 
					(g_module_state.host_type == AMC_FTRN_IN_LIBERA_SLOT_8) ||
					(g_module_state.host_type == AMC_FTRN_UNKNOWN_HOST);


		// Check conditions if ws packet can be sent out
		if (hs_event && in_libera){
		  // we definetly know we are in Libera, so delete HS event
		  printf("\nINFO: In Libera. Deleting event=0x%02X!\n", ws->pkt_out[4]);
		  ws_free( ws );
		  
		} else {
#endif
		//process outgoing events and responses
		  ws_set_state( ws, WS_ACTIVE_MASTER_WRITE_PENDING );
		  
		  if (hs_event) 
		    printf("WS: Sending event: type=0x%02X sensor=0x%02X!\n", ws->pkt_out[4], ws->pkt_out[6]);
		  
		  switch( ws->outgoing_medium ) {
			  case IPMI_CH_MEDIUM_IPMB:
				  i2c_master_write( ws );			
				  break;
				
			  case IPMI_CH_MEDIUM_SERIAL:	/* Asynch. Serial/Modem (RS-232) 	*/
			  case IPMI_CH_MEDIUM_ICMB10:	/* ICMB v1.0 				*/
			  case IPMI_CH_MEDIUM_ICMB09:	/* ICMB v0.9 				*/
			  case IPMI_CH_MEDIUM_LAN:	/* 802.3 LAN 				*/
			  case IPMI_CH_MEDIUM_LAN_AUX:	/* Other LAN				*/
			  case IPMI_CH_MEDIUM_PCI_SMB:	/* PCI SMBus				*/
			  case IPMI_CH_MEDIUM_SMB_1x:	/* SMBus v1.0/1.1			*/
			  case IPMI_CH_MEDIUM_SMB_20:	/* SMBus v2.0				*/
			  case IPMI_CH_MEDIUM_USB_1x:	/* reserved for USB 1.x			*/
			  case IPMI_CH_MEDIUM_USB_20:	/* reserved for USB 2.x			*/
			  case IPMI_CH_MEDIUM_SYS:	/* System Interface (KCS, SMIC, or BT)	*/
				  //dputstr( DBG_WS | DBG_ERR, "ws_process_work_list: unsupported protocol\n" );
				  ws_free( ws );
				  break;
			}
#ifdef LIBERA_HS_EVENT_HACK
		}
#endif
	}
	
	// process incomming request
	ws = ws_get_elem( WS_ACTIVE_MASTER_READ );
	if( ws ) {
		ws_set_state( ws, WS_ACTIVE_MASTER_READ_PENDING );
		if( ws->incoming_protocol == IPMI_CH_PROTOCOL_IPMB )
			i2c_master_read( ws );
	}
}

/* Default handler for incoming packets */
void
ws_process_incoming( IPMI_WS *ws )
{
	ipmi_process_pkt( ws );
	return;
}



/* print out state of elements in ws_array */
void
ws_print_state( unsigned max )
{
	unsigned i;
  unsigned max_read;

  if(max >= WS_ARRAY_SIZE){
    max_read = WS_ARRAY_SIZE-1;
  }else{
    max_read = max;
  }
	for ( i = 0; i < max_read; i++ )
	{
		printf("ws_array[%d].ws_state : %d\n",i, ws_array[i].ws_state);
	}
}


/* print out content of element [indx] in ws_array */
void
ws_print_content( unsigned indx )
{
  unsigned i, bound_indx;

  if(bound_indx >= WS_ARRAY_SIZE){
    bound_indx = WS_ARRAY_SIZE-1;
  }else{
    bound_indx = indx;
  }



  printf("------------------------\n\n");
  printf("\n\nws_array[%d]:@%08X\n",bound_indx, &ws_array[bound_indx]);
	printf("ws_array[%d]      : [HEX]\n",bound_indx);
	printf("ws_state         : %X\n",ws_array[bound_indx].ws_state         );  
	printf("len_rcv          : %X\n",ws_array[bound_indx].len_rcv          ); /* requested length of incoming pkt */
	printf("len_in           : %X\n",ws_array[bound_indx].len_in           ); /* lenght of incoming pkt */
	printf("len_out          : %X\n",ws_array[bound_indx].len_out          ); /* length of outgoing pkt */
	printf("len_sent         : %X\n",ws_array[bound_indx].len_sent         ); /* length of pkt actually sent */
	printf("timestamp        : %X\n",ws_array[bound_indx].timestamp        ); /* last access time to this ws element */
	printf("flags            : %X\n",ws_array[bound_indx].flags            ); /* protocol dependent i.e. WS_FL_xx */
	printf("addr_in          : %X\n",ws_array[bound_indx].addr_in          ); /* protocol dependent */
	printf("addr_out         : %X\n",ws_array[bound_indx].addr_out         ); 
	printf("incoming_channel : %X\n",ws_array[bound_indx].incoming_channel ); 
	printf("outgoing_channel : %X\n",ws_array[bound_indx].outgoing_channel ); 
	printf("incoming_protocol: %X\n",ws_array[bound_indx].incoming_protocol); 
	printf("outgoing_protocol: %X\n",ws_array[bound_indx].outgoing_protocol); 
	printf("incoming_medium  : %X\n",ws_array[bound_indx].incoming_medium  ); 
	printf("outgoing_medium  : %X\n",ws_array[bound_indx].outgoing_medium  ); 
	printf("interface        : %X\n",ws_array[bound_indx].interface        ); 
	printf("seq_out          : %X\n",ws_array[bound_indx].seq_out          ); /* sequence number */
	printf("delivery_attempts: %X\n",ws_array[bound_indx].delivery_attempts);
	printf("pkt addr         : %08X\n",&ws_array[bound_indx].pkt);        
        
	printf("pkt_in adr       : %08X\n",&ws_array[bound_indx].pkt_in);
	printf("pkt_out adr      : %08X\n",&ws_array[bound_indx].pkt_out);

  // print content of incoming packet
  printf("pkt_in:[\n");
  for(i=0; i < WS_BUF_LEN; i++)
  {
    printf("%02X ", ws_array[bound_indx].pkt_in[i]);
    if(((i+1) % 24) == 0 ) printf("#\n");
  }
  printf("]\n\n");

  // print content of outgoing packet
  printf("pkt_out:[\n");
  for(i=0; i < WS_BUF_LEN; i++)
  {
    printf("%02X ", ws_array[bound_indx].pkt_out[i]);
    if(((i+1) % 24) == 0 ) printf("#\n");
  }
  printf("]\n");
  printf("------------------------\n\n");

}




