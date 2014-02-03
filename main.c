    #include <stdio.h>
    #include <stdlib.h>

    #include "config.h"
    #include <plib/adc.h>
//    #include <plib/ctmu.h>
    #include <plib/i2c.h>

    #define USE_OR_MASKS

    struct {
    //    float adc;
    //    float humidutyWather;
    //    float humidutySoil;
 //       float capacitance;
        float temp;
        float humi;
        float temp2;
        unsigned int light;
        float diod1;
        float diod2;
    } data;

    struct {
        char maxTemp;
        char minTemp;
    } level = {40, 38};

    struct {
        char lampOn;
        char lampOff;
        signed int move;
    } run;


//sht11 required
//
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

unsigned int SoT;
unsigned int SoRh;
//sht11 required

/*void measurementTemp(void){

  float t[2];
  char error = 0;

  IdleI2C1();
  StartI2C1();

  IdleI2C1();
  if (WriteI2C1(0x90 & 0xfe) == -2){
	  error = 1;
  }

  IdleI2C1();
  if (WriteI2C1(0x00) == -2){
	  error = 1;
  }

  IdleI2C1();
  RestartI2C1();

  IdleI2C1();
  if (WriteI2C1(0x90 | 0x01)){
	  error = 1;
  }

  IdleI2C1();
  t[0] = ReadI2C1();
  AckI2C1();

  IdleI2C1();
  t[1] = ReadI2C1();
  IdleI2C1();
  NotAckI2C1();
  StopI2C1();

  data.temp = t[0]+(float)0xff/t[1];
}*/

/*    void measurementC(void){

      unsigned int Vread=0;
      float voltage;// Vcal=0, CTMUISrc = 0;
      float time = 15;
      float current = 0.55;//experiment time*current
      float ownCap = 0;//own capacitance
      float v = 3.25;

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

      voltage = (float)(v*Vread/1023.0);
      data.capacitance = (float)(time*current/voltage - ownCap);//result in pf
    }*/


void measure(char parametr){

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
		while(1);
	}

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

void calculation(){
	data.temp2=d2*SoT+d1;
	data.humi=(data.temp2-25)*(T1+T2*SoRh)+C1+C2*SoRh+C3*SoRh*SoRh;

//if(rh_true>100)rh_true=100;       //cut if the value is outside of
// if(rh_true<0.1)rh_true=0.1;       //the physical possible range

}

// измерение влажности воздуха и температуры
void measurementHumiTemp(){
  measure(MEASURE_TEMP);
  measure(MEASURE_HUMI);
  calculation();
}

 void measurementLight(void){
        IdleI2C();
        StartI2C();

        IdleI2C();
        if (WriteI2C(0x46) == -2){
//        if (WriteI2C(0xB8) == -2){
			//l-level 0xB8
			//h-level 0x46
          while(1);
        }

        IdleI2C();
        if (WriteI2C(0x13) == -2){
                //0x10 h-res
                //0x13 l-res
        while(1);
        }

        IdleI2C();
        StopI2C();

//		__delay_ms(80);
//		__delay_ms(80);
        __delay_ms(16);//for l-resolution

        IdleI2C();
        StartI2C();

        IdleI2C();
        if (WriteI2C(0x47) == -2){
//		if (WriteI2C(0xB9) == -2){
                //l-level 0x47
                //h-level 0xB9
                while(1);
        }

        IdleI2C();
        data.light = ReadI2C()<<8;
        AckI2C();

        IdleI2C();
        data.light |= ReadI2C();
        NotAckI2C();
        StopI2C();
        data.light = (float)data.light/1.2;
    }

int measurementADC(void){
  ADRESH=0;//clear the ADC result register
  ADRESL=0;//clear the ADC result register

  /* Read ADC*/
  ConvertADC(); // stop sampling and starts adc conversion
  while(BusyADC()); //wait untill the conversion is completed
  return ReadADC();//read the result of conversion
}

void measurementLightDiods(void){
	SetChanADC(ADC_CH0);
    data.diod1 = (float)measurementADC()*3.3/1023;
	__delay_ms(20);

	SetChanADC(ADC_CH1);
    data.diod2 = (float)measurementADC()*3.3/1023;
	__delay_ms(20);
}

void turnPlatform(int deg){
	static int position;
	int mov;

	if (deg > 180 && position > 0){
		deg = 180-deg;
	}
	
	mov = deg-position;
	position = (deg > 180)?180 - deg:deg;
}

/*void mLightDirection(){
	unsigned int i;
	for (i=0;i<=51;i++){

	}
}*/

void measurement(void){
  //измерение влажности воздуха и температуры датчиком sht1x
//      measurementHumiTemp();
//      measurementLight();
      measurementLightDiods();

  //измерение температуры
//      measurementTemp();
  //измерение влажности почвы
//  measurementC();

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
	if (data.temp < level.minTemp && !lampBurn()){
		run.lampOn = 0xff;
	} else if (data.temp > level.maxTemp && lampBurn()){
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
	LATBbits.LATB0 = 0;//for ligth sensor h-level address
}

void configI2C(void){
	TRISC = 0x18;//input rc3 and rc4
	SSP1ADD = 0xf9;					/* 100KHz (Fosc = 4MHz) */
//	OSCCON = 0b01101010;            // Fosc = 4MHz (Inst. clk = 1MHz)
	ANSELC = 0x0;					/* No analog inputs req' */

	OpenI2C1(MASTER, SLEW_OFF);
	DisableIntI2C1;
}

/*/void configCTMU(void){
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
  /
  //Set up AD converter;
  /
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
}*/

void configADC(){

    TRISA = 0x03;

//	unsigned char config1 = 0, config2 = 0, config3 = 0;

//	config1 = ADC_FOSC_2 | ADC_RIGHT_JUST | ADC_2_TAD;
//	config2 = ADC_CH0 | ADC_INT_OFF | ADC_REF_VDD_VSS;
//	config3 = ADC_2ANA;

//todo разобарться с этим
	ADCON0 = 0b00000001;
	ADCON1 = 0b00001101;
	ADCON2 = 0b10001000;
//	OpenADC(config1, config2, config3);
}

//function initializiation all components
void init(){

	OSCCONbits.IRCF0 = 0;
	OSCCONbits.IRCF1 = 1;
	OSCCONbits.IRCF2 = 1;//freq = 8MGz

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