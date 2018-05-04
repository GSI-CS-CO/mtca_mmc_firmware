/*
-------------------------------------------------------------------------------
coreIPM/iopin.c

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
#include "lpc21nn.h"
#include "iopin.h"
#include "mmcio.h"
#include "../project_defs.h"
#include "../payload.h"


void
iopin_set( unsigned long long bit )
{
	IOSET1 = ( unsigned )( bit >> 32 );
	IOSET0 = ( unsigned )bit;
}

void iopin_dir_out(unsigned long long bit){
	IODIR1 |= bit>>32;
	IODIR0 |= bit;
}

void iopin_dir_in(unsigned long long bit){
	IODIR1 &= ~(bit>>32);
	IODIR0 &= ~bit;
}

void
iopin_clear( unsigned long long bit )
{
	IOCLR1 = ( unsigned )( bit >> 32 );
	IOCLR0 = ( unsigned )bit;
}

/**
 *  iopin_led
 *
 *  fires the led
 *   on = 1  switch it on / light
 *      = 0  switch it off
 *
 *   active = 1  means led on
 *   		= 0  means led on
 */
void iopin_led(unsigned long long bit, int active, int on) {

	if (g_module_state.led_test_mode == LED_TEST_OFF){
		if (on == active) {
			iopin_set(bit);
		} else {
			iopin_clear(bit);
		}
	}else{
		if (g_module_state.led_test_mode == LED_TEST_ON_LED_ON){
			if (active) {
				iopin_set(bit);
			} else {
				iopin_clear(bit);
			}
		}else{ // LED_TEST_ON_LED_OFF
			if (active) {
				iopin_clear(bit);
			} else {
				iopin_set(bit);
			}
		}
	}
}

unsigned char
iopin_get( unsigned long long bit )
{
	unsigned char retval;

	if( bit >= 0x100000000 )
		retval = ( IOPIN1 & ( unsigned )( bit >> 32 ) ) ? 1: 0;
	else
		retval = ( IOPIN0 & ( unsigned )bit ) ? 1: 0 ;

	return retval;
}

/* set & reset IO bits simultaneously
 * Only bit positions which have a 1 in the mask will be changed */
void
iopin_assign( unsigned long long bit, unsigned long long mask )
{
	unsigned int reg0, reg1;

	reg0 = IOPIN0;
	reg1 = IOPIN1;

	IOPIN0 = ( reg0 & !mask ) | ( bit & mask );
	IOPIN1 = ( reg1 & !( mask >>32 ) ) | ( ( bit & mask ) >> 32 );
}

void iopin_initialize( void )
{

	/* Initialize Pin Connect Block */
	PINSEL0 =
		PS0_P0_0_TXD_UART_0 |
		PS0_P0_1_RXD_UART_0 |
		PS0_P0_2_SCL_I2C_0  |
		PS0_P0_3_SDA_I2C_0  |
		PS0_P0_4_SCK_0      |
		PS0_P0_5_MISO_0     |
		PS0_P0_6_MOSI_0     |
		PS0_P0_7_GPIO		| //	PS0_P0_7_EINT_2     |
		PS0_P0_8_GPIO		| //	PS0_P0_8_TDX_UART_1 |
		PS0_P0_9_GPIO		| //	PS0_P0_9_RDX_UART_1 |
		PS0_P0_10_GPIO		| //	PS0_P0_10_AD_1_2    |
		PS0_P0_11_SCL_I2C_1 |
		PS0_P0_12_GPIO      |
		PS0_P0_13_GPIO      |
		PS0_P0_14_SDA_I2C_1 |
		PS0_P0_15_GPIO;

	PINSEL1 =
		PS1_P0_16_GPIO   	|
		PS1_P0_17_GPIO      |
		PS1_P0_18_GPIO      |
		PS1_P0_19_GPIO      |
		PS1_P0_20_GPIO		| //	PS1_P0_20_EINT_3    |
		PS1_P0_21_GPIO		| //	PS1_P0_21_AD_1_6    |
		PS1_P0_22_GPIO		| //	PS1_P0_22_AD_1_7    |
		PS1_P0_23_GPIO      |
		PS1_P0_25_AD_0_4    |
		PS1_P0_28_AD_0_1    |
		PS1_P0_29_AD_0_2    |
		PS1_P0_30_AD_0_3    |
		PS1_P0_31_GPIO;		//	PS1_P0_31_AD_1_5;

	PINSEL2 =
		PS2_P1_16_25_GPIO   |
		PS2_P1_26_36_DEBUG;

	/* Set the default value & direction of each GPIO port pin, setting the bit makes it an output.
	 * Bit 0 - 31 in IO0DIR/IO0SET corresponds to P0.0 - P0.31.
	 * Bit 0 - 31 in IO1DIR/IO1SET corresponds to P1.0 - P1.31. */
	iopin_dir_out(PAYLOAD_PIN_POWER);
	iopin_clear( PAYLOAD_PIN_POWER );	// start with payload power off

	IODIR0 = ( unsigned int ) (
		P1					    |
		PAYLOAD_PIN_POWER	    |
		PAYLOAD_PIN_FPGA_CONFIG	|
		GPIO_2				    |
		LED_1				    |
		LED_0 );

//	IODIR1 = ( unsigned int ) ( 1<<16 ) |
//			(1<<24); //Set LED1 to output

	/* All LEDs OFF on startup */
	iopin_dir_out(P1);
	iopin_dir_out(LED_1);

	iopin_dir_out(LED_OK);
	iopin_dir_out(LED_ERROR);
	iopin_dir_out(LED_WHITE);
	iopin_dir_out(LED_USER);

	iopin_led(LED_OK,0,0);
	iopin_led(LED_ERROR,0,0);
	iopin_led(LED_WHITE,0,0);
	iopin_led(LED_USER,0,0);

	iopin_dir_out(PAYLOAD_PIN_QUISCE_OUT);
	iopin_clear(PAYLOAD_PIN_QUISCE_OUT);

	/* Payload pins configure */
	iopin_dir_out(PAYLOAD_PIN_FPGA_CONFIG);
	iopin_clear(PAYLOAD_PIN_FPGA_CONFIG);

	// hold PCIe reset until enabled via IPMI
	iopin_dir_out(PAYLOAD_PIN_PCIE_RESET);
	iopin_clear(PAYLOAD_PIN_PCIE_RESET);

	// hold Libera host pin low until enabled via IPMI
	iopin_dir_out(PAYLOAD_PIN_IN_LIBERA);
	iopin_clear(PAYLOAD_PIN_IN_LIBERA);

	// hold Libera Trigger enable pin low until enabled via IPMI
	iopin_dir_out(PAYLOAD_PIN_IN_SLOT8);
	iopin_clear(PAYLOAD_PIN_IN_SLOT8);

	// host type detect
	iopin_dir_in(STANDALONE);

	// sensor inputs
	iopin_dir_in(PAYLOAD_PIN_POWER_GOOD);
	iopin_dir_in(PAYLOAD_PIN_QUISCE_IN);
	iopin_dir_in(PAYLOAD_PIN_CONF_DONE);
	iopin_dir_in(PAYLOAD_PIN_MTCA4_EN);
	iopin_dir_in(PAYLOAD_PIN_JTAGSW_POS);

}
