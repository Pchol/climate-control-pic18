#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include <plib/adc.h>
#include <plib/ctmu.h>
#include <plib/i2c.h>

#define USE_OR_MASKS

struct {
//    float adc;
//    float humidutyWather;
//    float humidutySoil;
    char temp[2];
} data;

struct {
	char maxTemp;
	char minTemp;
} level = {40, 38};

struct {
	char lampOn;
	char lampOff;
} run;

void measurementTemp(void){

//  char data.temp;

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
  data.temp[0] = ReadI2C1();
  AckI2C1();

  IdleI2C1();
  data.temp[1] = ReadI2C1();
  IdleI2C1();
  NotAckI2C1();
  StopI2C1();
}

void measurement(){
  //измерение влажности почвы
  //измерение влажности воздуха
  //измерение температуры
  measurementTemp();

}

char lampBurn(void){
	if (!LATBbits.LATB5){
		return 0xff;
	} else {
		return 0;
	}
}

void compare(){
    //сравнение с показаниями и принятие решение какие устройства должны быть включены
    if (data.temp[0] < level.minTemp && !lampBurn()){
		run.lampOn = 0xff;
	} else if (data.temp[0] > level.maxTemp && lampBurn()){
		run.lampOff = 0xff;
	}
}

void execution(void){
    //исполнение
	if (run.lampOn){
		LATBbits.LATB5 = 0;
		run.lampOn = 0x0;
	} else if (run.lampOff){
		LATBbits.LATB5 = 1;
		run.lampOff = 0x0;
	}
}

void configI2C(void){
	TRISC = 0x18;//input rc3 and rc4 
//	SSP1ADD = 0x09;					/* 100KHz (Fosc = 4MHz) */
//	OSCCON = 0b01101010;            // Fosc = 4MHz (Inst. clk = 1MHz)
	ANSELC = 0x0;					/* No analog inputs req' */

	OpenI2C1(MASTER, SLEW_OFF);
	DisableIntI2C1;
}

void configPorts(void){
	TRISB = 0;
//	PORTB = 0;
}


//function initializiation all components
void init(){
    configPorts();
    configI2C();
//    configADC();
//    configCTMU();

}

//main
void main(void) {
    init();

    while(1){
        measurement();
        compare();
        execution();
		__delay_ms(20);
    }
}
