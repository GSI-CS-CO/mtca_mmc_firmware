/*
-------------------------------------------------------------------------------
coreIPM/mmcio.h

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

#include "arch.h"

// JTAG
// ----				  I/O				Pin
#define JTAG_TRS	P1_31	// I	JTAG TRST		20
#define JTAG_TMS	P1_30	// I	JTAG TMS		52
#define JTAG_TCK	P1_29	// I	JTAG TCK		56
#define JTAG_TDI	P1_28	// I	JTAG TDI		60
#define JTAG_TDO	P1_27	// O	JTAG TDO		64
#define JTAG_RTCK	P1_26	// I	JTAG RTCK		24					PRESENT_V12AUX
#define JTAG_EXTIN0	P1_25	// I	JTAG EXTIN		28
#define JTAG_TRACECLK	P1_24	// I	JTAG TRACECLK		32

// INTERRUPT GENERATING INPUTS
// ---------------------------
#define EINT_HOT_SWAP_HANDLE	P0_15  // EINT0							#MMC_HANDLE_SW
#define EINT_RESET		P0_7   // EINT2 - shared with PWM out			#MMC_SPI0_SEL_FLASH
#define EINT_SPI		P0_20  // EINT3									#MMC_PB_1

// NON-INTERRUPT, SWITCH INPUTS
// ----------------------------
#define HOT_SWAP_HANDLE		P0_15	// same function non-interrupt sensitive

// SPI
// ---
#define SCK_0		P0_4	// 27			MMC_SPI0_SCK
#define MISO_0		P0_5	// 29			MMC_SPI0_MISO
#define MOSI_0		P0_6	// 30			MMC_SPI0_MOSI

// ADC/GPIO
// --------
#define ADC_CH_0	P0_28	// 13				MMC_VS_V2_5			voltage sensor FPGA IO voltage
#define ADC_CH_1	P0_29	// 14 VSENSE12		MMC_VS_V12CAR		voltage sensor AMC 12V main rail from BPL or AUX connector
#define ADC_CH_2	P0_30	// 15				MMC_VS_V3_3_MP		voltage sensor BPL management power
#define ADC_CH_3	P0_25	//  9				MMC_VS_V1_15		voltage sensor FPGA voltage
#define ADC_CH_4	P0_10	// 35				MMC_PCIE_EN
#define ADC_CH_5	P0_15	// 45
#define ADC_CH_6	P0_26   //					MMC_VS_V1_15C		voltage sensor FPGA core voltage
#define ADC_CH_7	P0_27   //					MMC_VS_V3_3			voltage sensor FPGA IO voltage

// Alternate uses for ADC_CH_4 - 7 pins
#define SPI_SELECT_0	P0_10
#define SPI_SELECT_1	P0_15
#define SPI_SELECT_2	P0_21
#define SPI_SELECT_3	P0_22

/* Backplane addressing GPIO IN */
#define	BP_0		P1_16	// 16				#MMC_LED_FAIL
#define	BP_1		P1_17	// 12				#MMC_LED_OK
#define	BP_2		P1_18	//  8				#MMC_LED_FWRELOAD
#define	BP_3		P1_19	//  4				#MMC_ULED_0		This is input now, when low (0) board is inserted into crate, when high (1) board is standalone outside
#define	BP_4		P1_20	// 48				#MMC_ULED_1
#define	BP_5		P1_21	// 44				IPMI_GA_0
#define	BP_6		P1_22	// 40				IPMI_GA_1
#define	BP_7		P1_23	// 36				IPMI_GA_2


/* AMC GA0, GA1, GA2 lines GPIO IN - shared with backplane addressing functions */
#define GA0		P1_21	// 16
#define GA1		P1_22	// 12
#define GA2		P1_23	//  8

#define UART_0_TX	P0_0	// O	RS232 Transmit		19		MMC_UART_TX
#define UART_0_RX	P0_1	// I	RS232 Receive		21		MMC_UART_RX


#define I2C_0_SCL	P0_2	// I	I2C clock		22			IPMB_L_SCL
#define I2C_0_SDA	P0_3	// I/O	I2C data		26			IPMB_L_SDA


#define I2C_1_SCL	P0_11	// I	I2C clock		37			MMC_I2C_SCL
#define I2C_1_SDA	P0_14	// I/O	I2C data		41			MMC_I2C_SDA

#define UART_1_TX	P0_8	// O	RS232 Transmit		33		#MMC_SPI0_SEL_FPGA
#define UART_1_RX	P0_9	// I	RS232 Receive		34		JTAGSW_SEL


// General IO
#define	GPIO_0		P0_18	// 53
#define GPIO_2		P0_12	// 38
#define LED_1		P1_16	// 16 - RED LED
#define LED_0		P0_31	// 17 - BLUE LED					#MMC_LED_HOTSWAP
#define BLUE_LED	LED_0


#define LED1_RED	P1_16
#define LED2_GREEN  P1_17
#define LED2_RED  	P1_18

/*
 * These are the LEDS on AMC_FTRN board, controlled by LPC
 */
#define LED_ERROR	P1_17		// #MMC_LED_FAIL
#define LED_OK		P1_16		// #MMC_LED_OK		should represent the CONF_DONE pin
#define LED_WHITE	P1_18		// #MMC_LED_FWRELOAD, led blinks regarding the MAC_FTRN position

#define LED_USER 	P1_20		// #MMC_ULED_1		on the board, not visible on the front panel

#define STANDALONE	P1_19		// input, if 1 board is outside crate, if 0 board is in the crate

/* AMC P1 & PAYLOAD_POWER lines - GPIO OUT - shared with General IO */
#define P1			P1_24	// 53 GA_PULLUP															MMC_GA_COMMON

#define PAYLOAD_PIN_POWER       P0_12   //      User LED3 yellow, MS_PP_DCDC_EN on schematic, Denx	MMC_DCDC_EN
#define PAYLOAD_PIN_POWER_GOOD  P0_13   //      MMC_PGOOD

#define PAYLOAD_PIN_QUISCE_OUT  P0_18   //      MMC_QUIESCE_OUT
#define PAYLOAD_PIN_QUISCE_IN   P0_19   //      MMC_QUIESCE_IN

#define PAYLOAD_PIN_CONF_DONE	P1_25   // 28   CONF_DONE           FPGA boot status (HI when FPGA in user mode)
#define PAYLOAD_PIN_FPGA_CONFIG P0_17   // 47   MMC_FPGA_CONFIG     FPGA force FPGA reboot (active LO)

// Libera / M4 selection
#define PAYLOAD_PIN_MTCA4_EN	P0_16	// 46	FPGA2MMC_INT		MTCA.4 backplane buffer enable (active HI) indication from FPGA
							//							            LO - MTCA.4 buffers not enabled, HI - MTCA.4 buffers enabled
// Libera
#define PAYLOAD_PIN_IN_LIBERA   P0_21	//  1   MMC2FPGA_USR_1      LO - not in Libera, HI - when in Libera
#define PAYLOAD_PIN_IN_SLOT8    P0_22	//  2   MMC2FPGA_USR_2		Libera backplane buffer enable (active HI)
                                        //                          LO - when in Libera slot7, HI - when in Libera slot 8
#define PAYLOAD_PIN_PCIE_EN     P0_10	// 35	MMC_PCIE_EN         enable PCIe port - not used
#define PAYLOAD_PIN_PCIE_RESET	P0_23	// 58	#MMC_PCIE_RST		force PCIe core reset (check boot procedure for
                                        //                          Libera if/when PCIe core needs reset)
#define PAYLOAD_PIN_JTAGSW_POS  P0_9    // 58   JTAGSW_SEL          position of the JTAG switch select
