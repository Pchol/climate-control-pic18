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
    float capacitance;
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

void measurementC(void){

  unsigned int Vread=0;
  float voltage;// Vcal=0, CTMUISrc = 0;
  float time = 15;
  float current = 5.5;//experiment time*current
  float ownCap = 33.8;//own capacitance
  float v = 5.0;

  CTMUCONHbits.CTMUEN = 1; //Enable the CTMU
  CTMUCONLbits.EDG1STAT = 0; // Set Edge status bits to zero
  CTMUCONLbits.EDG2STAT = 0;
  CTMUCONHbits.IDISSEN = 1; //drain charge on the circuit
  __delay_us(15);
  CTMUCONHbits.IDISSEN = 0; //end drain of circuit

  CTMUCONLbits.EDG1STAT = 1;//Begin charging the circuit
  __delay_us(15);
  CTMUCONLbits.EDG1STAT = 0; 	 //Stop charging circuit

  PIR1bits.ADIF = 0; //make sure A/D Int not set
  ADCON0bits.GO=1; //and begin A/D conv.
  while(!PIR1bits.ADIF); //Wait for A/D convert complete
  PIR1bits.ADIF = 0; //Clear A/D Interrupt Flag
  
  Vread = ADRES;

  voltage = (float)(v*Vread/1024.0);
  data.capacitance = (float)(time*current/voltage - ownCap);//result in pf
}


void measurement(void){
  //измерение влажности почвы
  //измерение влажности воздуха
  //измерение температуры
  measurementTemp();
  measurementC();

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

void configPorts(void){
	TRISB = 0;
//	PORTB = 0;
}

void configI2C(void){
	TRISC = 0x18;//input rc3 and rc4 
//	SSP1ADD = 0x09;					/* 100KHz (Fosc = 4MHz) */
//	OSCCON = 0b01101010;            // Fosc = 4MHz (Inst. clk = 1MHz)
	ANSELC = 0x0;					/* No analog inputs req' */

	OpenI2C1(MASTER, SLEW_OFF);
	DisableIntI2C1;
}

void configCTMU(void){
  //CTMUCONH/1 - CTMU Control registers
  CTMUCONH = 0x00;
  //make sure CTMU is disabled
  CTMUCONL = 0x90;
  //CTMU continues to run when emulator is stopped,CTMU continues
  //to run in idle mode,Time Generation mode disabled, Edges are blocked
  //No edge sequence order, Analog current source not grounded, trigger
  //output disabled, Edge2 polarity = positive level, Edge2 source =
  //source 0, Edge1 polarity = positive level, Edge1 source = source 0,
  //CTMUICON - CTMU Current Control Register
  CTMUICON = 0x01;
  //0.55uA, Nominal - No Adjustment
  /**************************************************************************/
  //Set up AD converter;
  /**************************************************************************/
  TRISA=0x04;
  //set channel 2 as an input
  // Configure AN2 as an analog channel
  ANSELAbits.ANSA2=1;
  TRISAbits.TRISA2=1;
  // ADCON2
  ADCON2bits.ADFM=1;
  ADCON2bits.ACQT=1;
  ADCON2bits.ADCS=2;
  // ADCON1
  ADCON1bits.PVCFG0 =0;
  ADCON1bits.NVCFG1 =0;
  // ADCON0
  ADCON0bits.CHS=2;
  ADCON0bits.ADON=1;
}

//function initializiation all components
void init(){
    configPorts();
    configI2C();
    configCTMU();
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
