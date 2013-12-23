#include "p18f25k22.h"
#include "config.h"

#include <plib/i2c.h>

#define STATUS_REG_W 0x06   //000   0011    0
#define STATUS_REG_R 0x07   //000   0011    1
#define MEASURE_TEMP 0x03   //000   0001    1
#define MEASURE_HUMI 0x05   //000   0010    1
#define RESET_SENSOR 0x1e   //000   1111    0

const float C1=-4.0;              // for 12 Bit
const float C2=+0.0405;           // for 12 Bit
const float C3=-0.0000028;        // for 12 Bit
const float T1=+0.01;             // for 14 Bit @ 5V
const float T2=+0.00008;           // for 14 Bit @ 5V

const float d1=-39.66;
const float d2=0.01;

float t;
float ch;

unsigned int SoT;
unsigned int SoRh;

void measure(char parametr){

	char error = 0;
	char checksum;
	char *uk;

		//	select temp register
		IdleI2C1();
		StartI2C1();
		IdleI2C1();

		StopI2C1();
		IdleI2C1();
		RestartI2C1();
		IdleI2C1();

		if (WriteI2C1(parametr) == -2){
			error = 1;
		}

		if (!error){
			if (parametr == MEASURE_HUMI){
				__delay_ms(80);
				uk=(unsigned char)&SoRh;
			} else if (parametr == MEASURE_TEMP){
				__delay_ms(80);
				__delay_ms(80);
				__delay_ms(80);
				__delay_ms(80);
				uk=(unsigned char)&SoT;
			}

			IdleI2C1();
			*(uk+1)=ReadI2C1();
			AckI2C1();

			IdleI2C1();
			*(uk)=ReadI2C1();
			AckI2C1();

			IdleI2C1();
			checksum = ReadI2C1();
			NotAckI2C1();
	}
}

void calculation(){
	t=d2*SoT+d1;
	ch=(t-25)*(T1+T2*SoRh)+C1+C2*SoRh+C3*SoRh*SoRh;

//if(rh_true>100)rh_true=100;       //cut if the value is outside of
// if(rh_true<0.1)rh_true=0.1;       //the physical possible range

}

//полностью  рабочая программа
void main(void){

    OSCCONbits.IRCF0 = 0;
    OSCCONbits.IRCF1 = 1;
    OSCCONbits.IRCF2 = 1;

	TRISC = 0x18;
	SSP1ADD = 0xf9;					/* 100KHz (Fosc = 4MHz) */
//	OSCCON = 0b01101010;            // Fosc = 4MHz (Inst. clk = 1MHz)
	ANSELC = 0x0;					/* No analog inputs req' */

	OpenI2C1(MASTER, SLEW_OFF);
	DisableIntI2C1;

	while (1){

		measure(MEASURE_TEMP);
		measure(MEASURE_HUMI);
		calculation();
		__delay_ms(1);
	}
}
