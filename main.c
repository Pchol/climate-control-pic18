#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include <plib/adc.h>
//    #include <plib/ctmu.h>
#include <plib/i2c.h>
#include <plib/timers.h>

#define USE_OR_MASKS

  int  waitTurn = 0;
  int timerDeg;
  unsigned int timer1Deg = 0xF937;

    struct {
    //    float adc;
    //    float humidutyWather;
    //    float humidutySoil;
 //       float capacitance;
        float temp;
        float humi;
        unsigned int light;
        int degMaxLight;
	    struct {
			int maxDeg;
			int iterationDeg;
            int closeMeasurement;
			float diod1[36];
			float diod2[36];
        } lightPlatform;
    } data;

//{360, 10, 1}
    struct {
        char maxTemp;
        char minTemp;
		char maxHumi;
		char minHumi;
    } level = {40, 38, 25, 15};

    struct {
        char lampOn;
        char lampOff;
		char fanOn;
		char fanOff;
        signed int move;
    } run;

  void measure(char parametr);
  void mHumiTemp(void);
  void mLight(void);
  int mADC(void);
  void mLightPlatform(void);

  void init(void);
  void measurement(void);
  void compare(void);
  void execution(void);

  void configPorts(void);
  void configI2C(void);
  void configADC(void);
  void configTimer(void);

  int isLamp(void);
  int isTurn(void);
  int isFan(void);

  void interrupt TimerOverflow(void);
  int turnPlatform(int deg);

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

//function initializiation all components
void init(void){

	OSCCONbits.IRCF0 = 0;
	OSCCONbits.IRCF1 = 1;
	OSCCONbits.IRCF2 = 1;//freq = 8MGz

	ei();
	configPorts();
	configI2C();

    configADC();
	configTimer();

    data.lightPlatform.maxDeg = 360;
    data.lightPlatform.iterationDeg = 10;
    data.lightPlatform.closeMeasurement = 1;

//    configCTMU();

}

void measurement(void){
	static int one = 1;
	  //измерение освещенности датчиком
//      mLight();
	//  измерение влажности воздуха и температуры датчиком sht1x
//      mHumiTemp();
	  //измерение освещенности диодами
     if (!isTurn() && (!data.lightPlatform.closeMeasurement || one)){
		mLightPlatform();
        one = 0;
	 }

//	    measurementLightDiods();

  //измерение температуры
//      measurementTemp();
  //измерение влажности почвы
//  measurementC();

}

void compare(void){
    static int one = 0;
    if (!isTurn() && data.lightPlatform.closeMeasurement && !one){
        run.move = 1;
		one = 1;
    }
/*
    //сравнение с показаниями и принятие решение какие устройства должны быть включены
    if (data.temp < level.minTemp && !isLamp()){
            run.lampOn = 1;
    } else if (data.temp > level.maxTemp && isLamp()){
            run.lampOff = 1;
    }

    if (data.humi > level.maxHumi && !isFan()){
            run.fanOn = 1;
    } else if(data.humi < level.minHumi && isFan()) {
            run.fanOff = 1;
    }
 */
}

void execution(void){

	if (run.move){
		turnPlatform(360 - data.degMaxLight);//
		run.move = 0;
	}
	/*//исполнение
	if (run.lampOn){
		LATBbits.LATB5 = 0;
		run.lampOn = 0x0;
	} else if (run.lampOff){
		LATBbits.LATB5 = 1;
		run.lampOff = 0x0;
	}

	if (run.fanOn){
		LATCbits.LATC2 = 1;
		run.fanOn = 0x0;
	} else if (run.fanOff){
		LATCbits.LATC2 = 0;
		run.fanOff = 0x0;
	}*/
}

//measurement block

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

// measurement block
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
    StopI2C();
}

void calculation(void){
	data.temp=d2*SoT+d1;
	data.humi=(data.temp-25)*(T1+T2*SoRh)+C1+C2*SoRh+C3*SoRh*SoRh;

//if(rh_true>100)rh_true=100;       //cut if the value is outside of
// if(rh_true<0.1)rh_true=0.1;       //the physical possible range

}
//подпрограмма измерения макс угла освещенности
void mLightPlatform(void){
   	static int currentDeg = 0;

    if (data.lightPlatform.closeMeasurement == 1){//запуск
        data.lightPlatform.closeMeasurement = 0;
        currentDeg = 0;
    }
	if (currentDeg != data.lightPlatform.maxDeg){
		SetChanADC(ADC_CH0);
		data.lightPlatform.diod1[currentDeg/data.lightPlatform.iterationDeg] = (float)mADC()*3.3/1023;
		__delay_ms(20);

		SetChanADC(ADC_CH1);
		data.lightPlatform.diod2[currentDeg/data.lightPlatform.iterationDeg] = (float)mADC()*3.3/1023;
		__delay_ms(20);

		turnPlatform(data.lightPlatform.iterationDeg);
		currentDeg += data.lightPlatform.iterationDeg;
    } else if (!data.lightPlatform.closeMeasurement){ // окончание
        data.lightPlatform.closeMeasurement = 1;
		data.degMaxLight = calculateDeg();
    }
}

// измерение влажности воздуха и температуры
void mHumiTemp(void){
  measure(MEASURE_TEMP);
  measure(MEASURE_HUMI);
  calculation();
}

void mLight(void){
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

int mADC(void){
  ADRESH=0;//clear the ADC result register
  ADRESL=0;//clear the ADC result register

  /* Read ADC*/
  ConvertADC(); // stop sampling and starts adc conversion
  while(BusyADC()); //wait untill the conversion is completed
  return ReadADC();//read the result of conversion
}
// measurement block

//turn platform
//return 1 - left, 2 - right, 3 - wait 1 sec after, -1 error, 0 - free
int isTurn(void){
	if (!LATBbits.LATB4 && LATBbits.LATB2){
		return 1;
	} else if (!LATBbits.LATB2 && LATBbits.LATB4){
		return 2;
	} else if(!LATBbits.LATB2 && !LATBbits.LATB4){
		return -1;
    } else if (waitTurn == 1){
        return 3;
    } else {
		return 0;
	}
}

void interrupt TimerOverflow(void){
	if (INTCONbits.TMR0IF == 1){
		if (timerDeg > 0 && isTurn() != 2 && isTurn() != 3){
			timerDeg--;
			LATBbits.LATB4 = 0;
			WriteTimer0(timer1Deg);
		} else if (timerDeg < 0 && isTurn() != 1 && isTurn() != 3) {
			timerDeg++;
			LATBbits.LATB2 = 0;
			WriteTimer0(timer1Deg);
        } else {//off
            if (isTurn() != 3){
                waitTurn = 0;
            } else {
				LATBbits.LATB4 = 1;
				LATBbits.LATB2 = 1;
				waitTurn = 1;
            }
            //todo возможно стоит добавить задержку, ибо инерционность)
		}
		INTCONbits.TMR0IF = 0;
	}
}

int turnPlatform(int deg){
//	int mov;
//	unsigned int time = 0xE17B;//1 sec
//	unsigned int time = 0x0BD7;
//	unsigned int time = 0xA470;//3 sec
	//(4*256)/8×106 = 128uS
	//1/128uS = 7813 - 1s
   //организовать стек?
    if (!LATBbits.LATB4 || !LATBbits.LATB2){
        return 0;
    }
	timerDeg = deg;

	if (timerDeg > 0){
		LATBbits.LATB4 = 0;
	} else if (timerDeg < 0){
		LATBbits.LATB2 = 0;
	} 
//	WriteTimer0(0xfffe);
	INTCONbits.TMR0IF = 0;
    return 1;
}

int calculateDeg(void){
	int i;
	int val=0;
	int result=0;
	for (i = 0; i < 36; i++){
		if (data.lightPlatform.diod1[i] > val){
			val = data.lightPlatform.diod1[i];
			result = i;
		}
	}
	return result;
}
//turn platform

//check lamp is burn
int isLamp(void){
	if (!LATBbits.LATB5){
		return 1;
	} else {
		return 0;
	}
}

//fan
int isFan(void){
	if (LATCbits.LATC2){
		return 1;
	} else {
		return 0;
	}
}

//config
void configPorts(void){
	TRISB = 0;//all output
	TRISC = 0;//
//	исоплнительные механизмы
	LATCbits.LATC2 = 0;//fan default off
	LATBbits.LATB0 = 0;//for ligth sensor h-level address

	LATBbits.LATB2 = 1;
	LATBbits.LATB4 = 1;

}

void configI2C(void){
	TRISC = 0x18;//input rc3 and rc4
	SSP1ADD = 0xf9;					// 100KHz (Fosc = 4MHz)
//	OSCCON = 0b01101010;            // Fosc = 4MHz (Inst. clk = 1MHz)
	ANSELC = 0x0;					// No analog inputs req'

	OpenI2C1(MASTER, SLEW_OFF);
	DisableIntI2C1;
}

/*void configCTMU(void){
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

void configADC(void){

    TRISA = 0x03;

//	unsigned char config1 = 0, config2 = 0, config3 = 0;

//	config1 = ADC_FOSC_2 | ADC_RIGHT_JUST | ADC_2_TAD;
//	config2 = ADC_CH0 | ADC_INT_OFF | ADC_REF_VDD_VSS;
//	config3 = ADC_2ANA;

//todo разобарться с этим
	ADCON0 = 0b00000001;
	ADCON1 = 0b00001111;
	ADCON2 = 0b10001000;
//	OpenADC(config1, config2, config3);
}

void configTimer(void){

	T0CONbits.TMR0ON = 1;//enables timer
	T0CONbits.T08BIT = 0;//1 - 8bit 0 - 16 bit
	T0CONbits.PSA = 0;
	T0CONbits.T0PS0 = 1;//1:256
	T0CONbits.T0PS1 = 1;
	T0CONbits.T0PS2 = 1;
	T0CONbits.T0CS = 0;//internal

	TMR0H = 0;//reset
	TMR0L = 0;

	INTCONbits.TMR0IE = 1;//ienable timer0 overflow interrupt

//	OpenTimer0();
}
