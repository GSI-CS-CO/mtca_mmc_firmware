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
 * mmc_config.c
 *
 *  Created on: Aug 22, 2014
 *      Author: tom
 */

#include "coreIPM/ipmi.h"
#include "coreIPM/sensor.h"
#include "coreIPM/amc.h"
#include "payload.h"
#include "util/report.h"

#include "drivers/lm73.h"
#include "user_fru.h"
#include "fw_info.h"

#include <FreeRTOS.h>
#include <task.h>
#include <string.h>

/*
 * This file contains everything that needs to customized for MMC
 *
 * FRU
 *
 * Sensors
 */
void mmc_specific_sensor_init( void );
void mmc_specific_fru_init( void );

void mmc_config_init(){
	mmc_specific_fru_init();
	mmc_specific_sensor_init();
}



/* *************
 * FRU
 * *************/

extern FRU_CACHE fru_inventory_cache[];

struct fru_data {
	FRU_COMMON_HEADER hdr;
	//unsigned char internal[72];
	//unsigned char chassis[32];
	t_board_area_format_hdr board;
	t_product_area_format_hdr product;
	AMC_P2P_CONN_RECORD p2p_rec;
	AMC_CHANNEL_DESCRIPTOR amc_channel_0;
	AMC_LINK_DESCR amc_link_pcie;
	MODULE_CURRENT_REQUIREMENTS_RECORD mcr;
} fru_data;



void
mmc_specific_fru_init( void )
{
	// ====================================================================
	// initialize the FRU data records
	// - everything is cached for an AMC module
	// Note: all these are module specific
	// ====================================================================
	fru_inventory_cache[0].fru_dev_id = 0;
	fru_inventory_cache[0].fru_inventory_area_size = sizeof( fru_data );
	fru_inventory_cache[0].fru_data = ( unsigned char * )( &fru_data );

	/*
	 * Fru data header, no need to change anything in here....
	 */
	fru_data.hdr.format_version = 0x1;
	fru_data.hdr.int_use_offset = 0;	// not used currently
	fru_data.hdr.chassis_info_offset = 0;
	fru_data.hdr.board_offset = ( ( char * )&( fru_data.board ) - ( char * )&( fru_data ) ) >> 3;
	fru_data.hdr.product_info_offset = ( ( char * )&( fru_data.product ) - ( char * )&( fru_data ) ) >> 3;
	fru_data.hdr.multirecord_offset = ( ( char * )&( fru_data.p2p_rec ) - ( char * )&( fru_data ) ) >> 3;
	fru_data.hdr.pad = 0;
	fru_data.hdr.checksum = ipmi_calculate_checksum( ( unsigned char * )&( fru_data.hdr ), sizeof( FRU_COMMON_HEADER ) - 1 );


	// ====================================================================
    // BOARD DATA AREA
	fru_data.board.data.format_version 	= 0x1;
	fru_data.board.data.lang_code		= 0;
	fru_data.board.data.mfg_time[0]		= 0;
	fru_data.board.data.mfg_time[1]		= 0;
	fru_data.board.data.mfg_time[2]		= 0;
	fru_data.board.data.len				= (sizeof(t_board_area_format_hdr)) >> 3;


	strncpy((char*) &(fru_data.board.data.manuf), BOARD_MANUFACTURER, strlen(BOARD_MANUFACTURER));
	fru_data.board.data.manuf_len        = (strlen(BOARD_MANUFACTURER) & 0x3F);
	fru_data.board.data.manuf_type       = 0x03;

	strncpy((char*) &(fru_data.board.data.prod_name), BOARD_NAME,strlen(BOARD_NAME));
	fru_data.board.data.prod_name_len	= (strlen(BOARD_NAME) & 0x3F);
	fru_data.board.data.prod_name_type   = 0x03;

	strncpy((char*) &(fru_data.board.data.ser_num), BOARD_SN, strlen(BOARD_SN));
	fru_data.board.data.ser_num_len		= (strlen(BOARD_SN) & 0x3F);
	fru_data.board.data.ser_num_type   	= 0x03;

	strncpy((char*) &(fru_data.board.data.part_num), BOARD_PN, strlen(BOARD_PN));
	fru_data.board.data.part_num_len	= (strlen(BOARD_PN) & 0x3F);
	fru_data.board.data.part_num_type   = 0x03;

	strncpy((char*) &(fru_data.board.data.fru_file_id),FRU_FILE_ID , strlen(FRU_FILE_ID));
	fru_data.board.data.fru_file_id_len  = (strlen(FRU_FILE_ID) & 0x3F);
	fru_data.board.data.fru_file_id_type = 0x03;

	//fru_data.board.data.custom_data_len	= 0;
	//fru_data.board.data.custom_data_type= 0;

	fru_data.board.data.end_of_rec		= 0xC1;
	fru_data.board.checksum				= ipmi_calculate_checksum( ( unsigned char * )&( fru_data.board.data ), sizeof( t_board_area_format_hdr ) - 1 );


	// ====================================================================
	// PRODUCT DATA AREA
	fru_data.product.data.format_version	= 0x1;
	fru_data.product.data.lang_code			= 0;
	fru_data.product.data.len				= (sizeof(t_product_area_format_hdr)) >> 3;


	strncpy((char*) &(fru_data.product.data.manuf_name), PRODUCT_MANUFACTURER, strlen(PRODUCT_MANUFACTURER));
	fru_data.product.data.manuf_name_len        = (strlen(PRODUCT_MANUFACTURER) & 0x3F);
	fru_data.product.data.manuf_name_type       = 0x03;

	strncpy((char*) &(fru_data.product.data.prod_name), PRODUCT_NAME,strlen(PRODUCT_NAME));
	fru_data.product.data.prod_name_len	= (strlen(PRODUCT_NAME) & 0x3F);
	fru_data.product.data.prod_name_type   = 0x03;

	strncpy((char*) &(fru_data.product.data.prod_part_model_num), PRODUCT_PN, strlen(PRODUCT_PN));
	fru_data.product.data.prod_part_model_num_len		= (strlen(PRODUCT_PN) & 0x3F);
	fru_data.product.data.prod_part_model_num_type   	= 0x03;

	strncpy((char*) &(fru_data.product.data.prod_version), PRODUCT_VERSION, strlen(PRODUCT_VERSION));
	fru_data.product.data.prod_version_len		= (strlen(PRODUCT_VERSION) & 0x3F);
	fru_data.product.data.prod_version_type   	= 0x03;

	strncpy((char*) &(fru_data.product.data.prod_serial_num), PRODUCT_SN, strlen(PRODUCT_SN));
	fru_data.product.data.prod_serial_num_len	= (strlen(PRODUCT_SN) & 0x3F);
	fru_data.product.data.prod_serial_num_type   = 0x03;

	strncpy((char*) &(fru_data.product.data.asset_tag), PRODUCT_TAG, strlen(PRODUCT_TAG));
	fru_data.product.data.asset_tag_len		= 0; //(strlen(PRODUCT_TAG) & 0x3F);
	fru_data.product.data.asset_tag_type   	= 0x00;

	strncpy((char*) &(fru_data.product.data.fru_file_id),FRU_FILE_ID , strlen(FRU_FILE_ID));
	fru_data.product.data.fru_file_id_len  = (strlen(FRU_FILE_ID) & 0x3F);
	fru_data.product.data.fru_file_id_type = 0x03;

/*	strncpy((char*) &(fru_data.product.data.custom_data_0),PRODUCT_CUSTOM_DATA_0 , strlen(PRODUCT_CUSTOM_DATA_0));
	fru_data.product.data.custom_data_0_len  = (strlen(PRODUCT_CUSTOM_DATA_0) & 0x3F);
	fru_data.product.data.custom_data_0_type = 0x03;
*/

	strncpy((char*) &(fru_data.product.data.custom_data_0),MMC_FW_INFO_0 , strlen(MMC_FW_INFO_0));
	fru_data.product.data.custom_data_0_len  = (strlen(MMC_FW_INFO_0) & 0x3F);
	fru_data.product.data.custom_data_0_type = 0x03;

	strncpy((char*) &(fru_data.product.data.custom_data_1),MMC_FW_INFO_1 , strlen(MMC_FW_INFO_1));
	fru_data.product.data.custom_data_1_len  = (strlen(MMC_FW_INFO_1) & 0x3F);
	fru_data.product.data.custom_data_1_type = 0x03;

	strncpy((char*) &(fru_data.product.data.custom_data_2),MMC_FW_INFO_2 , strlen(MMC_FW_INFO_2));
	fru_data.product.data.custom_data_2_len  = (strlen(MMC_FW_INFO_2) & 0x3F);
	fru_data.product.data.custom_data_2_type = 0x03;

	strncpy((char*) &(fru_data.product.data.custom_data_3),MMC_FW_INFO_3 , strlen(MMC_FW_INFO_3));
	fru_data.product.data.custom_data_3_len  = (strlen(MMC_FW_INFO_3) & 0x3F);
	fru_data.product.data.custom_data_3_type = 0x03;

	strncpy((char*) &(fru_data.product.data.custom_data_4),MMC_FW_INFO_4 , strlen(MMC_FW_INFO_4));
	fru_data.product.data.custom_data_4_len  = (strlen(MMC_FW_INFO_4) & 0x3F);
	fru_data.product.data.custom_data_4_type = 0x03;

	fru_data.product.data.end_of_rec		= 0xC1;
	fru_data.product.checksum				= ipmi_calculate_checksum( ( unsigned char * )&( fru_data.product.data ), sizeof( t_product_area_format_hdr ) - 1 );



	/*
	 * E-Keying support:
	 *
	 * a point to point record includes all of the details required to describe
	 * FRU specific fabric usage. Modify fru structure to include as many different channel
	 * and link descriptors needed.
	 */

	//NO NEED TO CHANGE THIS PART
	/* Point-to-point connectivity record */
	fru_data.p2p_rec.record_type_id = 0xC0;	/* For all records a value of C0h (OEM) shall be used. */
	fru_data.p2p_rec.eol = 0;		/* [7:7] End of list. Set to one for the last record */
	fru_data.p2p_rec.reserved = 0;		/* Reserved, write as 0h.*/
	fru_data.p2p_rec.version = 2;		/* record format version (2h for this definition) */
	/* Manufacturer ID - For the AMC specification the value 12634 (00315Ah) must be used. */
	fru_data.p2p_rec.manuf_id[0] = 0x5A;
	fru_data.p2p_rec.manuf_id[1] = 0x31;
	fru_data.p2p_rec.manuf_id[2] = 0x00;
	fru_data.p2p_rec.picmg_rec_id = 0x19;	/* 0x19 for AMC Point-to-Point Connectivity record */
	fru_data.p2p_rec.rec_fmt_ver = 0;	/* Record Format Version, = 0 for this specification */
	fru_data.p2p_rec.oem_guid_count = 0;	/* OEM GUID Count */
	fru_data.p2p_rec.record_type = 1;	/* 1 = AMC Module */
	fru_data.p2p_rec.conn_dev_id = 0;	/* Connected-device ID if Record Type = 0, Reserved, otherwise. */

	//FRU Specific
	fru_data.p2p_rec.ch_descr_count = 1;	/* AMC Channel Descriptor Count */
	fru_data.p2p_rec.record_len = 8+(1*3)+(1*5);	/* Record Length: 8 + channel count * 3 + link count * 5 */

	/*
	 * Channel 0 PCIe
	 */
	fru_data.amc_channel_0.lane_0_port_num = 4;		//Port 0 used
	fru_data.amc_channel_0.lane_1_port_num = 0x1f;	//Not used
	fru_data.amc_channel_0.lane_2_port_num = 0x1f;	//Not used
	fru_data.amc_channel_0.lane_3_port_num = 0x1f;	//Not used
	fru_data.amc_channel_0.reserved = 0xf; 			//Reserved

	fru_data.amc_link_pcie.reserved=0x3f; 			//Reserved
	fru_data.amc_link_pcie.asym_match = 0x01;		//MATCHES_01
	fru_data.amc_link_pcie.link_grouping_id =0;		//Single channel
	fru_data.amc_link_pcie.link_type = AMC_LINK_PCI_EXPRESS;		//PCIe
	fru_data.amc_link_pcie.link_type_ext = 0x0;
	fru_data.amc_link_pcie.lane_0_bit_flag=1;		//Lane 0 used
	fru_data.amc_link_pcie.lane_1_bit_flag=0;		//Lane 1 unused
	fru_data.amc_link_pcie.lane_2_bit_flag=0;		//Lane 2 unused
	fru_data.amc_link_pcie.lane_3_bit_flag=0;		//Lane 3 unused
	fru_data.amc_link_pcie.amc_channel_id = 0;		//Use channel 0

	fru_data.p2p_rec.record_cksum = ipmi_calculate_checksum( ( unsigned char * )&( fru_data.p2p_rec.manuf_id[0] ), fru_data.p2p_rec.record_len );
	fru_data.p2p_rec.header_cksum = ipmi_calculate_checksum( ( unsigned char * )&( fru_data.p2p_rec.record_type_id ), 4 );


	/*
	 * Current requirements record
	 */
	fru_data.mcr.rec_type_id = 0xc0;
	fru_data.mcr.end_list = 1;	/* End of List. Set to one for the last record */
	fru_data.mcr.rec_format = 0x2;	/* Record format version (= 2h for this definition) */
	fru_data.mcr.rec_length = 0x6;	/* Record Length */
	fru_data.mcr.manuf_id_lsb = 0x5A;
	fru_data.mcr.manuf_id_midb = 0x31;
	fru_data.mcr.manuf_id_msb = 0x00;
	fru_data.mcr.picmg_rec_id = 0x16; /* PICMG Record ID. For the Module Power
					     Descriptor table, the value 16h must be used. */
	fru_data.mcr.rec_fmt_ver = 0;	/* Record Format Version. As per AMC specification,
					   the value 0h must be used. */
	fru_data.mcr.curr_draw = 15;	/* Current Draw = 1.5A. In units of 0.1A at 12V */
	fru_data.mcr.rec_cksum = ipmi_calculate_checksum( ( unsigned char * )&( fru_data.mcr.manuf_id_lsb ), fru_data.mcr.rec_length );
	fru_data.mcr.hdr_cksum = ipmi_calculate_checksum( ( unsigned char * )&( fru_data.mcr.rec_type_id ), 4 );


}


/* *************
 * SENSORS
 * *************/


FULL_SENSOR_RECORD voltage_payload_12_sdr;
SENSOR_DATA voltage_payload_12_sd;
void sensor_read_12V(void * sd){
	((SENSOR_DATA *)sd)->last_sensor_reading=g_module_state.sensors.PP12V/55;
}

FULL_SENSOR_RECORD voltage_mgmt_3v3_sdr;
SENSOR_DATA voltage_mgmt_3v3_sd;
void sensor_read_3V3(void * sd){
	((SENSOR_DATA *)sd)->last_sensor_reading=g_module_state.sensors.MP3V3/16;
}

FULL_SENSOR_RECORD temp1_sdr;
SENSOR_DATA temp1_sd;
void sensor_read_t1(void * sd){
	((SENSOR_DATA *)sd)->last_sensor_reading=g_module_state.sensors.TEMP1/430;
}

FULL_SENSOR_RECORD temp2_sdr;
SENSOR_DATA temp2_sd;
void sensor_read_t2(void * sd){
	((SENSOR_DATA *)sd)->last_sensor_reading=g_module_state.sensors.TEMP2/430;
}

FULL_SENSOR_RECORD fpga_config_sdr;
SENSOR_DATA fpga_config_sd;
void sensor_read_fpga_config(void *sd) {
	((SENSOR_DATA *)sd)->last_sensor_reading=g_module_state.sensors.FPGA_CONFIG;
}

FULL_SENSOR_RECORD conf_done_sdr;
SENSOR_DATA conf_done_sd;
void sensor_read_conf_done(void *sd) {
	((SENSOR_DATA *)sd)->last_sensor_reading=g_module_state.sensors.CONF_DONE;
}

FULL_SENSOR_RECORD trig_libera_sdr;
SENSOR_DATA trig_libera_sd;
void sensor_read_trig_libera(void *sd) {
	((SENSOR_DATA *)sd)->last_sensor_reading=g_module_state.sensors.TRIG_LIBERA;
}

FULL_SENSOR_RECORD trig_mtca4_sdr;
SENSOR_DATA trig_mtca4_sd;
void sensor_read_trig_mtca4(void *sd) {
	((SENSOR_DATA *)sd)->last_sensor_reading=g_module_state.sensors.TRIG_MTCA4;
}

FULL_SENSOR_RECORD jtag_sw_pos_sdr;
SENSOR_DATA jtag_sw_pos_sd;
void sensor_read_jtag_sw_pos(void *sd) {
	((SENSOR_DATA *)sd)->last_sensor_reading=g_module_state.sensors.JTAGSW_POS;
}


void tsk_temp_monitor();

void
mmc_specific_sensor_init( void )
{

	info("MMC_CONFIG","Sensors & SDR Init");

//	unsigned char dev_slave_addr =  module_get_i2c_address( I2C_ADDRESS_LOCAL );;

	/** Payload power voltage sensor
	 *
	 * Raw reading is returned in millivolts (from ADC)
	 * IPMI Sensor factors:
	 *
	 * 	we have 1 byte for raw data, sensor range is 0-14V
	 * 	Therefore we have resolution of 14000 (mV) /255 == 54.9(mV per LSB)
	 * 	mV to V == factor -3, therfore K2 factor is -3
	 */
	generic_sensor_init(&voltage_payload_12_sdr,"+12V Payload",ST_VOLTAGE);

	voltage_payload_12_sdr.M=55; //Set M factor to
	voltage_payload_12_sdr.R_B_exp=-3<<4; //Set K2
	voltage_payload_12_sdr.sensor_threshold_access = 0b01; // thresholds are readable, per Reading Mask
	voltage_payload_12_sdr.reading_mask= 0b00111111; // all thresholds readable

	voltage_payload_12_sdr.lower_non_critical_threshold    = 183;   //  10V  - min input
	voltage_payload_12_sdr.lower_critical_threshold        = 164;   //   9V
	voltage_payload_12_sdr.lower_non_recoverable_threshold = 109;   //   6V

	voltage_payload_12_sdr.upper_non_critical_threshold    = 236;   //  13V
	voltage_payload_12_sdr.upper_critical_threshold        = 245;   //  13.5V
	voltage_payload_12_sdr.upper_non_recoverable_threshold = 250;   //  13.7V - max input MMC ADC is 14V

	voltage_payload_12_sdr.ignore_sensor=0;
	voltage_payload_12_sdr.modifier_unit = 0b00; // 00b = none

	voltage_payload_12_sd.scan_function=sensor_read_12V;
	voltage_payload_12_sd.unavailable=0;
	voltage_payload_12_sd.sensor_scanning_enabled=1;
	voltage_payload_12_sd.signed_flag=0;
	sensor_add(&voltage_payload_12_sdr,&voltage_payload_12_sd);


	/** Management power voltage sensor
	 *
	 * Raw reading is returned in milivolts (from ADC)
	 * IPMI Sensor factors:
	 *
	 * 	we have 1 byte for raw data, sensor range is 0-4.1V
	 * 	Therefore we have resolution of 4100 (mV) /255 == 16.01(mV per LSB)
	 * 	mV to V == factor -3, therefore K2 factor is -3
	 */
	generic_sensor_init(&voltage_mgmt_3v3_sdr,"+3.3V Mng",ST_VOLTAGE);
	voltage_mgmt_3v3_sdr.M=16;
	voltage_mgmt_3v3_sdr.R_B_exp=-3<<4;
	voltage_mgmt_3v3_sdr.ignore_sensor=0;
	voltage_mgmt_3v3_sdr.modifier_unit = 0b00; // 00b = none

	voltage_mgmt_3v3_sdr.sensor_threshold_access = 0b01; // thresholds are readable, per Reading Mask
	voltage_mgmt_3v3_sdr.reading_mask= 0b00111111; // all thresholds readable

	voltage_mgmt_3v3_sdr.lower_non_critical_threshold    = 193;   //  3.1V  - min input for LPC213x is 3.0V
	voltage_mgmt_3v3_sdr.lower_critical_threshold        = 196;   //  3.15V
	voltage_mgmt_3v3_sdr.lower_non_recoverable_threshold = 109;   //   6V

	voltage_mgmt_3v3_sdr.upper_non_critical_threshold    = 212;   //  3.4V
	voltage_mgmt_3v3_sdr.upper_critical_threshold        = 218;   //  3.5V  - max input for LPC213x is 3.6V
	voltage_mgmt_3v3_sdr.upper_non_recoverable_threshold = 222;   //  3.58V - max input for LPC213x is 3.6V

	voltage_mgmt_3v3_sd.scan_function=sensor_read_3V3;
	voltage_mgmt_3v3_sd.unavailable=0;
	voltage_mgmt_3v3_sd.sensor_scanning_enabled=1;
	voltage_mgmt_3v3_sd.signed_flag=0;
//	voltage_mgmt_3v3_sd.ignore_sensor=0;
	sensor_add(&voltage_mgmt_3v3_sdr,&voltage_mgmt_3v3_sd);

	/** Temperature sensor 1
	 *
	 * Raw reading is returned in degC (from ADC)
	 * IPMI Sensor factors:
	 *
	 * 	we have 1 byte for raw data, sensor range is 0-11000 (representing 110.00 degC)
	 * 	Therefore we have resolution of 110,00 (degC) /255 == 0.431 (degC per LSB)
	 * 	mV to V == factor -2, therefore K2 factor is -2
	 */
	generic_sensor_init(&temp1_sdr,"Temp Inlet",ST_TEMPERATURE);
	temp1_sd.unavailable=0;
	temp1_sd.sensor_scanning_enabled=1;
	temp1_sdr.sensor_units2 = SENSOR_UNIT_DEGREES_CELSIUS;
	temp1_sdr.M=43;
	temp1_sdr.R_B_exp=-2<<4;
	temp1_sdr.ignore_sensor=0;
	temp1_sdr.modifier_unit = 0b00; // 00b = none

	temp1_sdr.sensor_threshold_access = 0b01; // thresholds are readable, per Reading Mask
	temp1_sdr.reading_mask= 0b00111111; // all thresholds readable

	temp1_sdr.lower_non_critical_threshold    =  11; //  5°C
	temp1_sdr.lower_critical_threshold        =   4; //  2°C
	temp1_sdr.lower_non_recoverable_threshold =   0; //  0°C

	temp1_sdr.upper_non_critical_threshold    = 139; // 60°C
	temp1_sdr.upper_critical_threshold        = 162; // 70°C
	temp1_sdr.upper_non_recoverable_threshold = 185; // 80°C

	temp1_sd.scan_function=sensor_read_t1;
	temp1_sd.signed_flag = 1;
	sensor_add(&temp1_sdr,&temp1_sd);

	generic_sensor_init(&temp2_sdr,"Temp Outlet",ST_TEMPERATURE);
	temp2_sd.unavailable=0;
	temp2_sd.sensor_scanning_enabled=1;
	temp2_sdr.sensor_units2 = SENSOR_UNIT_DEGREES_CELSIUS;
	temp2_sdr.M=43;
	temp2_sdr.R_B_exp=-2<<4;
	temp2_sdr.ignore_sensor=0;
	temp2_sdr.modifier_unit = 0b00; // 00b = none

	temp2_sdr.sensor_threshold_access = 0b01; // thresholds are readable, per Reading Mask
	temp2_sdr.reading_mask= 0b00111111; // all thresholds readable

	temp2_sdr.lower_non_critical_threshold    =  11; //  5°C
	temp2_sdr.lower_critical_threshold        =   4; //  2°C
	temp2_sdr.lower_non_recoverable_threshold =   0; //  0°C

	temp2_sdr.upper_non_critical_threshold    = 139; // 60°C
	temp2_sdr.upper_critical_threshold        = 162; // 70°C
	temp2_sdr.upper_non_recoverable_threshold = 185; // 80°C

	temp2_sd.scan_function=sensor_read_t2;
	temp2_sd.signed_flag = 1;
	sensor_add(&temp2_sdr,&temp2_sd);

	/** Discrete sensors:
	 *
	 *		FPGA_CONFIG
	 *		CONF_DONE
	 *		LIBERA TRIGGER
	 *      MTCA4 TRIGGER
	 *      JTAG SWITCH POS
	 *
	 */
	generic_sensor_init(&fpga_config_sdr,"FPGA CONFIG",IPMI_LIBERA_SENSOR_TYPE);
	fpga_config_sd.unavailable=0;
	fpga_config_sd.sensor_scanning_enabled=1;
	fpga_config_sdr.event_type_code = EVT_TYPE_CODE_SENSOR_SPECIFIC;
	fpga_config_sdr.ignore_sensor=0;
	fpga_config_sd.scan_function =sensor_read_fpga_config;
	sensor_add(&fpga_config_sdr,&fpga_config_sd);

	generic_sensor_init(&conf_done_sdr,"CONF DONE",IPMI_LIBERA_SENSOR_TYPE);
	conf_done_sd.unavailable=0;
	conf_done_sd.sensor_scanning_enabled=1;
	conf_done_sdr.event_type_code = EVT_TYPE_CODE_SENSOR_SPECIFIC;
	conf_done_sdr.ignore_sensor=0;
	conf_done_sd.scan_function =sensor_read_conf_done;
	sensor_add(&conf_done_sdr,&conf_done_sd);

	generic_sensor_init(&trig_libera_sdr,"LIBERA TRIGGER",IPMI_LIBERA_SENSOR_TYPE);
	trig_libera_sd.unavailable=0;
	trig_libera_sd.sensor_scanning_enabled=1;
	trig_libera_sdr.event_type_code = EVT_TYPE_CODE_SENSOR_SPECIFIC;
	trig_libera_sdr.ignore_sensor=0;
	trig_libera_sd.scan_function = sensor_read_trig_libera;
	sensor_add(&trig_libera_sdr,&trig_libera_sd);

	generic_sensor_init(&trig_mtca4_sdr,"MTCA4 TRIGGER",IPMI_LIBERA_SENSOR_TYPE);
	trig_mtca4_sd.unavailable=0;
	trig_mtca4_sd.sensor_scanning_enabled=1;
	trig_mtca4_sdr.event_type_code = EVT_TYPE_CODE_SENSOR_SPECIFIC;
	trig_mtca4_sdr.ignore_sensor=0;
	trig_mtca4_sd.scan_function = sensor_read_trig_mtca4;
	sensor_add(&trig_mtca4_sdr,&trig_mtca4_sd);

	generic_sensor_init(&jtag_sw_pos_sdr,"JTAGSW POS",IPMI_LIBERA_SENSOR_TYPE);
	jtag_sw_pos_sd.unavailable=0;
	jtag_sw_pos_sd.sensor_scanning_enabled=1;
	jtag_sw_pos_sdr.event_type_code = EVT_TYPE_CODE_SENSOR_SPECIFIC;
	jtag_sw_pos_sdr.ignore_sensor=0;
	jtag_sw_pos_sd.scan_function = sensor_read_jtag_sw_pos;
	sensor_add(&jtag_sw_pos_sdr,&jtag_sw_pos_sd);


	info("MMC_CONFIG","Sensors added: %d",nof_sensors_added());

	xTaskCreate(tsk_temp_monitor,"TMP MON",configMINIMAL_STACK_SIZE,0,tskIDLE_PRIORITY+1,0);

}


/*
 * LM73 sensor structures and defintions
 */
struct lm73_ws g_LM73_TEMP1 = { .address = 0x49 };
struct lm73_ws g_LM73_TEMP2 = { .address = 0x4a };


/*
 * Monitor task, executed periodically on low priority
 */
void tsk_temp_monitor() {
	info("TEMP_MON", "Temp mon task start");
	lm73_init(&g_LM73_TEMP1);
	lm73_init(&g_LM73_TEMP2);
	while (1) {
		lm73_readTemp(&g_LM73_TEMP1);
		g_module_state.sensors.TEMP1 = g_LM73_TEMP1.last_reading;

		lm73_readTemp(&g_LM73_TEMP2);
		g_module_state.sensors.TEMP2 = g_LM73_TEMP2.last_reading;

		//debug(3,"SENSOR_READ","T1: %d T2: %d", g_module_state.sensors.TEMP1, g_module_state.sensors.TEMP2);

		/*
		 * when and if comes to implementation of ADC voltage sensors
		 * code for voltage readings will be inserted here
		 *
		 *  ADC_CH_0	P0_28	// 13				MMC_VS_V2_5			voltage sensor FPGA IO voltage
		 *  ADC_CH_1	P0_29	// 14 VSENSE12		MMC_VS_V12CAR		voltage sensor AMC 12V main rail from BPL or AUX connector
		 *  ADC_CH_2	P0_30	// 15				MMC_VS_V3_3_MP		voltage sensor BPL management power
		 *  ADC_CH_3	P0_25	//  9				MMC_VS_V1_15		voltage sensor FPGA voltage
		 *  ADC_CH_6	P0_26   //					MMC_VS_V1_15C		voltage sensor FPGA core voltage
		 *  ADC_CH_7	P0_27   //					MMC_VS_V3_3			voltage sensor FPGA IO voltage
		 *
		 * Currently this is not implemented
		 */
		vTaskDelay(1000);
	}
}


