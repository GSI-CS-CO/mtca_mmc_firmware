/*
-------------------------------------------------------------------------------
coreIPM/sensor.h

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


/* Assertion Event Codes */
#define ASSERTION_EVENT                 0x00
#define DEASSERTION_EVENT               0x80

/* Sensor States */
#define SENSOR_STATE_NORMAL             0x00    // temperature is in normal range
#define SENSOR_STATE_LOW                0x01    // temperature is below lower non critical
#define SENSOR_STATE_LOW_CRIT           0x02    // temperature is below lower critical
#define SENSOR_STATE_LOW_NON_REC        0x04    // temperature is below lower non recoverable
#define SENSOR_STATE_HIGH               0x08    // temperature is higher upper non critical
#define SENSOR_STATE_HIGH_CRIT          0x10    // temperature is higher upper critical
#define SENSOR_STATE_HIGH_NON_REC       0x20    // temperature is higher high non recoverable

/* IPMI Sensor Events */
#define IPMI_THRESHOLD_LNC_GL           0x00    // lower non critical going low
#define IPMI_THRESHOLD_LNC_GH           0x01    // lower non critical going high
#define IPMI_THRESHOLD_LC_GL            0x02    // lower critical going low
#define IPMI_THRESHOLD_LC_GH            0x03    // lower critical going HIGH
#define IPMI_THRESHOLD_LNR_GL           0x04    // lower non recoverable going low
#define IPMI_THRESHOLD_LNR_GH           0x05    // lower non recoverable going high
#define IPMI_THRESHOLD_UNC_GL           0x06    // upper non critical going low
#define IPMI_THRESHOLD_UNC_GH           0x07    // upper non critical going high
#define IPMI_THRESHOLD_UC_GL            0x08    // upper critical going low
#define IPMI_THRESHOLD_UC_GH            0x09    // upper critical going HIGH
#define IPMI_THRESHOLD_UNR_GL           0x0A    // upper non recoverable going low
#define IPMI_THRESHOLD_UNR_GH           0x0B    // upper non recoverable going high


typedef struct sensor_data {
	uchar	sensor_id; 
	uchar	last_sensor_reading;
	uchar 	discrete;
	uchar	scan_period;		/* time between each sensor scan in seconds, 0 = no scan */
	uchar	state;
	uchar	old_state;
	uchar	signed_flag;
	void(*scan_function)( void * );	/* the routine that does the sensor scan */
	FULL_SENSOR_RECORD *sdr;

#ifdef BF_MS_FIRST
	uchar	event_messages_enabled:1,	/* 0b = All Event Messages disabled from this sensor */
		sensor_scanning_enabled:1,	/* 0b = sensor scanning disabled */
		unavailable:1,			/* 1b = reading/state unavailable */
		reserved;
#else
	uchar	reserved:5,
		unavailable:1,
		sensor_scanning_enabled:1,
		event_messages_enabled:1;
#endif 
    struct {
		uchar upper_non_recoverable_go_high:1;
		uchar upper_non_recoverable_go_low:1;
		uchar upper_critical_go_high:1;
		uchar upper_critical_go_low:1;
		uchar upper_non_critical_go_high:1;
		uchar upper_non_critical_go_low:1;
		uchar lower_non_recorverable_go_high:1;
		uchar lower_non_recoverable_go_low:1;
		uchar lower_critical_go_high:1;
		uchar lower_critical_go_low:1;
		uchar lower_non_critical_go_high:1;
        uchar lower_non_critical_go_low:1;
    } asserted_event;
} SENSOR_DATA;

typedef struct sdr_entry {
	unsigned short	record_id;
	uchar	rec_len;
	uchar	*record_ptr;
} SDR_ENTRY;


void ipmi_get_device_sdr_info( IPMI_PKT *pkt );
void ipmi_get_device_sdr( IPMI_PKT *pkt );
void ipmi_reserve_device_sdr_repository( IPMI_PKT *pkt );
void ipmi_get_sensor_reading( IPMI_PKT *pkt );
void ipmi_get_sensor_reading_factors( IPMI_PKT *pkt );
void ipmi_get_sensor_threshold( IPMI_PKT *pkt );
int  compact_sensor_add( COMPACT_SENSOR_RECORD *sdr,SENSOR_DATA *sensor_data );
int  sensor_add( FULL_SENSOR_RECORD *sdr, SENSOR_DATA *sensor_data ); 
void generic_sensor_init(FULL_SENSOR_RECORD* sr, char* sensor_name, uchar sensor_type);
void sdr_add(void* sdr, uchar sdr_size);

int nof_sensors_added(void);
