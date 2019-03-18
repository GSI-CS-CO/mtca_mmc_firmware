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


#define FTRN_LIBERA_TRIGGER  0xF0

#define FTRN_CMD_PCIE_RESET  1


/*
 * Host type state of AMC_FTRN card
 */
#define AMC_FTRN_HOST_NOT_EVALUATED 0x00
#define AMC_FTRN_UNKNOWN_HOST       0x01
#define AMC_FTRN_IN_LIBERA_SLOT_7 	0x07
#define AMC_FTRN_IN_LIBERA_SLOT_8   0x08
#define AMC_FTRN_IN_MICROTCA        0x11
#define AMC_FTRN_IN_MICROTCA_4      0x44
#define AMC_FTRN_OUTSIDE            0x77


#define LED_TEST_OFF         0x0
#define LED_TEST_ON_LED_OFF  0x1
#define LED_TEST_ON_LED_ON   0x2


enum amc_payload_state{
	PAYLOAD_IDLE,
	PAYLOAD_WAIT_FOR_12V,
	PAYLOAD_WAIT_FOR_PG,
	PAYLOAD_WAIT_FOR_CONF_DONE,
	PAYLOAD_ACTIVE,
	PAYLOAD_ACTIVE_PCIE_RESET,
	PAYLOAD_ACTIVE_PCIE_RESET_DEASSERT,
	PAYLOAD_ACTIVE_BLINK_GREEN,
	PAYLOAD_WAIT_SHUTDOWN,
	PAYLOAD_ERR
};

/*
 * Contains all of the current information about payload
 */
struct module_status{
	enum amc_payload_state payload_state;
	unsigned char payload_activate;
	unsigned char force_active;
	unsigned char led_test_mode;

	unsigned char host_type;
	unsigned char ipmi_amc_host;
	unsigned char cmd;

	struct sensors{
		int PP12V; //Payload 12V [mV]
		int MP3V3; //Management power [mV]
		int TEMP1; //Temp1 LM73 [mDegC]
		int TEMP2; //Temp1 LM73 [mDegC]
		unsigned char POWERGOOD;   // Powergood [bool]
		unsigned char FPGA_CONFIG; // signal for fpga configuration
		unsigned char CONF_DONE;   // FPGA configuration complete [bool]
//		unsigned char INIT_DONE;   // FPGA init done [bool] - maybe in future
		unsigned char QUIESCE;     // FPGA response to Quiesce
		unsigned char TRIG_LIBERA; // Libera triggers == in slot 8
		unsigned char TRIG_MTCA4;  // MTCA.4 triggers and clocks
		unsigned char JTAGSW_POS;  // position of the JTAG switch
	} sensors;

	struct ports{	//Current port status (is port enabled and routed to backplane)
					//Port number corresponds to channel number defined in FRU (mmc_config.c)
		unsigned
			PCIe:1,
			LiberaTrig:1,
			Mtca4:1;
	} ports;
}g_module_state;


void payload_init();

//Activate payload boot up
void payload_activate();

//Attempt to shutdown the payload
void payload_quiesce();

void payload_port_set_state(unsigned char link_type, char *link_type_name,unsigned char port,unsigned char enable);

void enterIdle(void);



