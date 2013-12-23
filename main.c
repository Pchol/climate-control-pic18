#include "p18f25k22.h"
#include "config.h"

#include <plib/i2c.h>

//полностью  рабочая программа
void main(void){

	char data[2];
	TRISC = 0x18;
//	SSP1ADD = 0x09;					/* 100KHz (Fosc = 4MHz) */
//	OSCCON = 0b01101010;            // Fosc = 4MHz (Inst. clk = 1MHz)
	ANSELC = 0x0;					/* No analog inputs req' */

	OpenI2C1(MASTER, SLEW_OFF);
	DisableIntI2C1;

//	select temp register
	IdleI2C1();
	StartI2C1();

	IdleI2C1();
	WriteI2C1(0x90 & 0xfe);//1001000 1001101

	IdleI2C1();
	WriteI2C1(0x00);

	IdleI2C1();
	RestartI2C1();

	IdleI2C1();
	WriteI2C1(0x90 | 0x01);

	IdleI2C1();
	data[0] = ReadI2C1();
	AckI2C1();

	IdleI2C1();
	data[1] = ReadI2C1();
	IdleI2C1();
	NotAckI2C1();
	StopI2C1();

   	while(1);
}