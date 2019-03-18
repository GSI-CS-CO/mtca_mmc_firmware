/*
-------------------------------------------------------------------------------
coreIPM/debug.c

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


#include "stdio.h"
#include "stdarg.h"
#include "string.h"
#include "debug.h"
#include "ipmi.h"
#include "strings.h"

unsigned global_debug_setting = 0;//DBG_IPMI | DBG_SERIAL | DBG_I2C | DBG_WD | DBG_TIMER | DBG_LAN | DBG_GPIO | DBG_WS | DBG_ERR | DBG_LVL1 | DBG_LVL2 | DBG_WARN ;//| DBG_INOUT;

#ifdef USE_DPRINTF
void
dprintf(unsigned flags, char *fmt, ...)
{
	printf("dprintf\n");
	va_list argp;
	if( ( ( flags & global_debug_setting ) >> 8 )  && ( ( flags & global_debug_setting ) & 0x0f ) ) {
		va_start( argp, fmt );
		vprintf( fmt, argp );
		va_end( argp );
	}
}
#endif

void
putstr( char *str )
{
	int i, len;
	
	len = strlen( str );
	for( i = 0; i < len; i++ ) {
		putchar( str[i] );
	}
}

void
dputstr( unsigned flags, char *str)
{
/*
	if( ( ( flags & global_debug_setting ) >> 8 )  && ( ( flags & global_debug_setting ) & 0x0f ) ) {
		putstr( str );
	}
*/
	//printf((const char) *str);
	//info("dputstr",(const char) *str);
}

void
puthex( unsigned char ch )
{
	unsigned char hexc;

	hexc = (ch >> 4) & 0x0F;
	hexc += (hexc < 10) ? '0' : ('A'- 10);
	putchar(hexc);

	hexc = ch & 0x0F;
	hexc += (hexc < 10) ? '0' : ('A' - 10);
	putchar(hexc);

}

#define I2C_STAT_LOG_LEN 256

unsigned char i2c_state_log[I2C_STAT_LOG_LEN][2];
unsigned log_wr_index = 0;
unsigned log_rd_index = 0;


void log_i2c_status(unsigned cmd_stat, unsigned data){
  i2c_state_log[log_wr_index][0] = cmd_stat;
  i2c_state_log[log_wr_index][1] = data;

  log_wr_index++;
  log_wr_index = log_wr_index % I2C_STAT_LOG_LEN;
}

void list_i2c_status_log(void){
  unsigned i;
  unsigned index;
  //log_rd_index = log_wr_index + 1;

  printf("\nLast write index: %03d/%d\n",log_wr_index-1,I2C_STAT_LOG_LEN-1);
  for (i = 0; i < I2C_STAT_LOG_LEN; i++){
    index = (log_wr_index + i) % I2C_STAT_LOG_LEN;
    printf("%03d\t%02X\t%02X \n",index, i2c_state_log[index][0], i2c_state_log[index][1]);
  }
}


