#define USE_FIFO
/*
 -------------------------------------------------------------------------------
 coreIPM/serial.c

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

/* Universal Asynchronous Receiver Transmitter 0 & 1 (UART0/1) registers */
/*
 UART0 UART1
 ----- -----
 U0RBR U1RBR  Receiver Buffer Register
 U0THR U1THR  Transmit Holding Register
 U0IER U1IER  Interrupt Enable Register
 U0IIR U1IIR  Interrupt Identification Register
 U0FCR U1FCR  FIFO Control Register
 U0LCR U1LCR  Line Control Register
 U0MCR U1MCR  Modem control reg
 U0LSR U1LSR  Line Status Register
 U0MSR U1MSR  Modem status reg
 U0SCR U1SCR  Scratch pad register
 U0DLL U1DLL  Divisor Latch Registers (LSB)
 U0DLM U1DLM  Divisor Latch Registers (MSB)
 U0ACR U1ACR  Auto-baud Control Register
 U0FDR U1FDR  Fractional Divider Register
 U0TER U1TER  Transmit Enable Register
 */

/* 
 GENERAL OPERATION
 -----------------
 The UART0 receiver block, U0RX, monitors the serial input line, RXD0, for valid
 input. The UART0 RX Shift Register (U0RSR) accepts valid characters via RXD0.

 After a valid character is assembled in the U0RSR, it is passed to the UART0
 RX Buffer Register FIFO to await access by the CPU or host via the generic
 host interface.

 The UART0 transmitter block, U0TX, accepts data written by the CPU or host and
 buffers the data in the UART0 TX Holding Register FIFO (U0THR). The UART0 TX
 Shift Register (U0TSR) reads the data stored in the U0THR and assembles the data
 to transmit via the serial output pin, TXD0.

 The UARTn Baud Rate Generator block, U0BRG, generates the timing enables used
 by the UART0 TX block. The U0BRG clock input source is the VPB clock (PCLK).
 The main clock is divided down per the divisor specified in the U0DLL and U0DLM
 registers. This divided down clock is a 16x oversample clock, NBAUDOUT.

 The interrupt interface contains registers U0IER and U0IIR. The interrupt interface
 receives several one clock wide enables from the U0TX and U0RX blocks.
 Status information from the U0TX and U0RX is stored in the U0LSR. Control information
 for the U0TX and U0RX is stored in the U0LCR.

 The modem interface contains registers U1MCR and U1MSR. This interface is
 responsible for handshaking between a modem peripheral and the UART1.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "arch.h"
#include "../util/ringbuffer.h"
#include "../util/report.h"

#include <FreeRTOS.h>
#include <semphr.h>
#include <task.h>

#define uchar unsigned char

#define CR     0x0D
#define LF     0x0A
#define CLI_PROMPT "BMC>"
#define RX_BUFFER_SIZE 128
#define TX_BUFFER_SIZE 128

ringBuffer_typedef(unsigned char, ucharBuffer_t);

struct uart_ws {
	/*
	 * Receive circular buffer
	 */
	ucharBuffer_t rx_buffer;
	/*
	 * Transmitt circular buffer
	 */
	ucharBuffer_t tx_buffer;
	SemaphoreHandle_t uart_lock;
};

struct uart_ws g_UART0_WS;

/*==============================================================*/
/* Local Function Prototypes					*/
/*==============================================================*/
/* UART ISR */
void UART0_ISR(void) __attribute__ ((interrupt));
//void UART_ISR_1(void) __attribute__ ((interrupt));

#define UARTINT_MODEM		0x00	// Modem interrupt
#define UARTINT_ERROR		0x06	// RX Line Status/Error
#define UARTINT_RX_DATA_AVAIL	0x04	// Rx data avail or trig level in FIFO
#define UARTINT_CHAR_TIMEOUT	0x0C	// Character Time-out indication	      
#define UARTINT_THRE		0x02	// Transmit Holding Register Empty
#define UARTINT_ENABLE_RX_DATA	0x001	// Enable the RDA interrupts.
#define UARTINT_ENABLE_THRE	0x002	// Enable the THRE interrupts.
#define UARTINT_ENABLE_ERROR	0x004	// Enable the RX line status interrupts.
#define UARTINT_ENABLE_MODEM	0x008	// Enable the modem interrupt.
#define UARTINT_ENABLE_CTS	0x080	// Enable the CTS interrupt.
#define UARTINT_ENABLE_AUTO_BAUD	0x100	// Enable Auto-baud Time-out Interrupt.
#define UARTINT_ENABLE_END_AUTO_BAUD	0x200	// Enable End of Auto-baud Interrupt.
#define UART_LSR_RX_DATA_RDY		0x01
#define UART_LSR_OVERRUN_ERROR		0x02
#define UART_LSR_PARITY_ERROR		0x04
#define UART_LSR_FRAMING_ERROR		0x08
#define UART_LSR_BREAK_INTERRUPT	0x10
#define UART_LSR_TX_HOLDING_REG_EMPTY	0x20
#define UART_LSR_TX_EMPTY		0x40
#define UART_LSR_RX_FIFO_ERROR		0x80

#define UART_FCR_FIFO_ENABLE	0x01
#define UART_FCR_FIFO_1_CHAR	0x00
#define UART_FCR_FIFO_4_CHAR	0x40
#define UART_FCR_FIFO_8_CHAR	0x80
#define UART_FCR_FIFO_14_CHAR	0xC0

/*==============================================================
 * uart_initialize()
 *==============================================================*/
void uart_init(void) {
	/* Init work structures */
	bufferInit(g_UART0_WS.rx_buffer, RX_BUFFER_SIZE, unsigned char);
	bufferInit(g_UART0_WS.tx_buffer, TX_BUFFER_SIZE, unsigned char);
	g_UART0_WS.uart_lock = xSemaphoreCreateMutex();

	/* Initialize VIC */
	/* Interrupt Select register (VICIntSelect) is a read/write accessible 
	 * register. This register classifies each of the 32 interrupt requests
	 * as contributing to FIQ or IRQ.
	 * 0 The interrupt request with this bit number is assigned to the IRQ category.
	 * 1 The interrupt request with this bit number is assigned to the FIQ category. */
//	VICIntSelect = 0x0; /* assign all interrupt reqs to the IRQ category */

	/* Symbol  Function  PINSEL0 bit
	 P0.0  = TxD0      [1:0]
	 P0.1  = RxD0      [3:2] */
	PINSEL0 |= 0x00000005; /* Enable RxD0 and TxD0 */
	U0LCR = 0x83; /* 8 bits, no Parity, 1 Stop bit */
	U0DLM = 0;

	/* 12e6 / (16 * 6 * (1 + (1 / 12))) = approx. 115384.62
	 * DLL = 6
	 * DIVADDVAL = 1
	 * DIVMULVAL = 12
	 */
  U0DLL = 6; /* 115200 Baud Rate @ 12MHz VPB Clock */
	//U0DLL = 12; /* 57600 Baud Rate @ 12MHz VPB Clock */

	U0FDR = (12 << 4) | (1); /* Bits 7:4 == MULVAL, 4:0 == ADDVAL */

	U0LCR = 0x03; /* Disable access to Divisor Latches */

//	U0IER = UARTINT_ENABLE_RX_DATA | /* Enable the RDA interrupts */
//	UARTINT_ENABLE_THRE; /* Enable transmit register empty interrupts */
//	U0FCR = UART_FCR_FIFO_ENABLE | UART_FCR_FIFO_4_CHAR;

//	VICVectAddr5 = (unsigned long) UART0_ISR; /* set interrupt vector in 5 */
//	VICVectCntl5 = 0x20 | IS_UART0; /* use it for UART0 interrupt */
//	VICIntEnable = IER_UART0; /* enable UART1 interrupt */

//	/* P0.8  = TxD1      [17:16]
//	   P0.9  = RxD1      [19:18]
//	   P0.10 = RTS1      [21:20] (NC on connector, avail on board)
//	   P0.11 = CTS1      [23:22] (NC on connector, avail on board) */
//	PINSEL0 |= 0x00050000;	/* Enable RxD1 and TxD1 */
//	U1LCR = 0x83;		/* 8 bits, no Parity, 1 Stop bit */
//	U1DLL = 78;		/* 9600 Baud Rate @ 12MHz VPB Clock */
//	U1LCR = 0x03;		/* Disable access to Divisor Latches */
//	U1IER = UARTINT_ENABLE_RX_DATA;   /* Enable the RDA interrupts */
//#ifdef USE_FIFO
//	U1FCR = UART_FCR_FIFO_ENABLE | UART_FCR_FIFO_4_CHAR;
//#endif
//	VICVectAddr4 = ( unsigned long )UART_ISR_1;	/* set interrupt vector in 4 */
//	VICVectCntl4 = 0x20 | IS_UART1;			/* use it for UART1 interrupt */
//	VICIntEnable = IER_UART1;			/* enable UART1 interrupt */
//
//	serial_port[0].port_name = UART_DEBUG;
//	serial_port[0].filter_type = UART_FILTER_TERM;
//	serial_port[0].callback_fn = term_process;
//
//	serial_port[1].port_name = UART_ITLA;
//	serial_port[1].filter_type = UART_FILTER_RAW;
//	serial_port[1].callback_fn = 0;

}

void UART0_ISR(void) {
	uchar int_reg;
	uchar tmp_char;
	unsigned char temp;
	ucharBuffer_t* rxBuffer_ptr = &(g_UART0_WS.rx_buffer);
	ucharBuffer_t* txBuffer_ptr = &(g_UART0_WS.tx_buffer);

	int_reg = U0IIR;	// read U0IIR to clear UART interrupt flag

	switch (int_reg & 0x0f) {
	case UARTINT_ERROR:		// TODO: error handling
		temp = U0LSR;		// clear interrupt
		break;
	case UARTINT_RX_DATA_AVAIL:	// Rx char available
	case UARTINT_CHAR_TIMEOUT:	// Character Time-out indication
		while ( U0LSR & UART_LSR_RX_DATA_RDY) {
			tmp_char = U0RBR;	// get char and reset int
			bufferWrite(rxBuffer_ptr, tmp_char);
		}
		break;
	case UARTINT_THRE:		// Transmit Holding Register Empty
		//Fill the hw FIFO transmit buffer
		while (U0LSR & UART_LSR_TX_HOLDING_REG_EMPTY) {
			//No more data
			if (isBufferEmpty(txBuffer_ptr)) {
				break;
			}
			bufferRead(txBuffer_ptr, tmp_char);
			U0THR = tmp_char;
		}
		break;			// U1IIR Read has reset interrupt
	case UARTINT_MODEM:
		temp = U0MSR;		// clear interrupt
		break;
	}

	VICVectAddr = 0;          		// Acknowledge Interrupt
}

void uart0_putchar(char c) {

//	xSemaphoreTake(g_UART0_WS.uart_lock,10);


	if(c=='\n') uart0_putchar('\r');

	ucharBuffer_t* txBuffer_ptr = &(g_UART0_WS.tx_buffer);
	bufferWrite(txBuffer_ptr, c);

	if(isBufferFull(txBuffer_ptr)){
		bufferWrite(txBuffer_ptr,'!');
	}

	//check if transmission is running?
	if (U0LSR & UART_LSR_TX_EMPTY) {
		//Fill the hw FIFO transmit buffer
		while (U0LSR & UART_LSR_TX_HOLDING_REG_EMPTY) {
			//No more data
			if (isBufferEmpty(txBuffer_ptr)) {
				break;
			}
			bufferRead(txBuffer_ptr, c);
			U0THR = c;
		}
	}

	//	xSemaphoreGive(g_UART0_WS.uart_lock);

}

int putchar_0( int ch )	// Write character to Serial Port 0
{
	if ( ch == '\n' )  {
		while ( !( U0LSR & 0x20 ) );	// spin while Transmitter Holding Register not Empty
		U0THR = CR;			// output CR
	}

	while ( !( U0LSR & 0x20 ) );

	return ( U0THR = ch );
}

unsigned char uart0_haschar(){
	ucharBuffer_t* rxBuffer_ptr = &(g_UART0_WS.rx_buffer);;

	return isBufferEmpty(rxBuffer_ptr);
}

char uart0_getchar(){
	ucharBuffer_t* rxBuffer_ptr = &(g_UART0_WS.rx_buffer);;

	if(!uart0_haschar()) return 0;

	char c;
	bufferRead(rxBuffer_ptr,c);
	return c;
}

void uart0_emptyrx(){
	ucharBuffer_t* rxBuffer_ptr = &(g_UART0_WS.rx_buffer);;
	char tmp;
	while(!isBufferEmpty(rxBuffer_ptr)) bufferRead(rxBuffer_ptr,tmp);
}


// Simple polled routines
/*
 putchar()

 Parameters
 ch - Character to be written. The character is passed as its int promotion.

 Return Value
 If there are no errors, the same character that has been written is returned.
 If an error occurs, EOF is returned.
 */

//int
//#if defined (__CA__) || defined ( __GNUC__ )
//putchar( int ch )	// Write character to the debug serial port
//#elif defined (__CC_ARM)
//sendchar( int ch )	// Write character to the debug serial port
//#endif
//{
//	switch( UART_DEBUG ) {
//		case UART_0:
//			return ( putchar_0( ch ) );
//		case UART_1:
//			return ( putchar_1( ch ) );
//	}
//	return EOF;
//}
/*
 putc()

 Writes a character to the stream.

 If there are no errors, the same character that has been written is returned.
 If an error occurs, EOF is returned and the error indicator is set.
 */
/*
 int
 putc( int ch, int handle )
 {
	 if ( !(( handle !=  0) || ( handle != 1 ) ) ) {
		 return( EOF );
	 }

	 switch( serial_port[handle].port_name ) {
	 case UART_0:
		 putchar_0( ch );
		 break;
	 case UART_1:
		 putchar_1( ch );
		 break;
	 default:
		 return( EOF );
		 break;
	 }

	 return( ch );
 }
*/

/*
 fputs()

 On success, a non-negative value is returned.
 On error, the function returns EOF.
 */
/*
 int
 fputs ( const char * str, int handle )
 {
 int i, len, ret;

 len = strlen( str );
 for( i = 0; i < len; i++ ) {
 ret = putc( str[i], handle );
 if( ret != 1 )
 return( EOF );
 }
 return( ESUCCESS );
 }
 */

/*
 fflush()

 On success, a non-negative value is returned.
 On error, the function returns EOF.
 */
/*
 int
 fflush( int handle )
 {
 switch( handle ) {
 case 0:
 buf_index_0 = 0;
 break;
 case 1:
 buf_index_1 = 0;
 break;
 default:
 return( EOF );
 }
 return( ESUCCESS );
 }
 */

