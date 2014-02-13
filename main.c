#include "config.h"
#include <plib/adc.h>
//    #include <plib/ctmu.h>
#include <plib/i2c.h>
#include <plib/timers.h>
#include <plib/pwm.h>

#define USE_OR_MASKS


    struct {
        float temp;
        float humi;
        unsigned int light;
        int degMaxLight;
	    struct {
			int maxDeg;
			int iterationDeg;
			int startMeasurement;
            int closeMeasurement;
			float diod1[36];
			float diod2[36];
        } lightPlatform;
	} data = {0, 0, 0, 0, {360, 10, 1}};

    struct {
        char maxTemp;
        char minTemp;
		char maxHumi;
		char minHumi;
    } level = {40, 38, 25, 30};

    struct {
        char lampOn;
        char lampOff;
		char fanOn;
		char fanOff;
		int dumpOn;
		int dumpOff;
    } run;
	
	struct {
		struct {
			int time;
			int on;
			int complete;
		} platform;
		
		struct {
			int time;
			int on;
			int complete;
		} dump;
		
		struct {
			int time;
			int on;
			int complete;
		} temp;
		
	} event;

  int mSht1(char parametr);
  unsigned int mLight(void);
  int mADC(void);
  void mLightPlatform(void);
  
  int calcPWM(void);//определяем яркость освещения
  int calcMaxDeg(void);//определяем угол максимальной освещенности
  void calcSht1(float *humi, float *temp);

  void init(void);
  void measurement(void);
  void compare(void);
  void execution(void);

  void configPorts(void);
  void configI2C(void);
  void configADC(void);
  void configTimers(void);
  void configCCP(void);

  int isLamp(void);
  int isTurn(void);
  int isFan(void);
  int isDump(void);

  int turnPlatform(int deg);
  void offPlatform(void);

//main
void main(void) {
	init();

	while(1){//check events
		if (event.platform.complete){
			if (!data.lightPlatform.closeMeasurement){
				mLightPlatform();
				event.platform.time = data.lightPlatform.iterationDeg;

                if (data.lightPlatform.closeMeasurement){//измерение закончено, ждем 1 секунду
                    event.platform.time = 5;//one sec wait
                }
				event.platform.on = 1;
			} else {
				if (!isTurn()){
                    int returnDeg = -(data.lightPlatform.maxDeg - data.degMaxLight);
                    if (returnDeg < 0){
						turnPlatform(-1);
                        event.platform.time = returnDeg;
						event.platform.on = 1;
                    }
				} else {
					offPlatform();
					//не устанавливаем event.platform.on, на этом заканчиваем работу с платформой
				}
			}
			event.platform.complete = 0;
		}

		if (event.dump.complete){
			//самое время запустить насос или остановить
			if (!isDump()){
				LATBbits.LATB1 = 0;
                event.dump.time = 15;

                event.dump.on = 1;
			} else if (isDump()){
					LATBbits.LATB1 = 1;
					//only one
			}
			event.dump.complete = 0;
		}

		if (event.temp.complete){
			
			measurement();//производим измерения всех датчков
			compare();//сравниваем значения с уровнями
			execution();//запускаем устройства

            event.temp.time = 1;
			event.temp.complete = 0;
			event.temp.on = 1;
		//измерение влажности воздуха и температуры датчиком sht1x
		//измерение освещенности диодами
		}
	}
}
//function initializiation all components
void init(void){

	OSCCONbits.IRCF0 = 0;
	OSCCONbits.IRCF1 = 1;
	OSCCONbits.IRCF2 = 1;//freq = 8MGz

	ei();//включаем прерывания
	configPorts();
	configI2C();

    configADC();
	configTimers();

    configCCP();
//    configCTMU();

	while(!TMR2IF);

	TRISBbits.RB3 = 0;
	
	event.dump.time = 20;
	event.dump.on = 1;

	event.temp.time = 1;
	event.temp.on = 1;

	event.platform.time = 1;
	event.platform.on = 1;

}

void measurement(void){
	  //измерение освещенности датчиком
	data.temp = mSht1(0);//изм. темпр.
	data.humi = mSht1(1);//из. влажность
	calcSht1(&data.temp, &data.humi);//делаем перерасчет

    data.light = mLight();
//измерение температуры
//  measurementTemp();
//измерение влажности почвы
//  mC();

}

void compare(void){
	
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
}

void execution(void){

	//исполнение
	if (run.lampOn){
		LATBbits.LATB5 = 0;
		run.lampOn = 0;
	} else if (run.lampOff){
		LATBbits.LATB5 = 1;
		run.lampOff = 0;
	}

	if (run.fanOn){
		LATCbits.LATC2 = 1;
		run.fanOn = 0;
	} else if (run.fanOff){
		LATCbits.LATC2 = 0;
		run.fanOff = 0;
	}

	SetDCEPWM2(calcPWM());
}

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

/*    void mC(void){

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

/* функция проводящая измерения влажности/температуры воздуха
 * char param - параметр, определяющий что будем измерять (0 - влажность, 1 - температура)
 * измеренные значения сохраняются в data.codeTemp или data.codeHumi
 * return возвращает измеренные значения
 */
int mSht1(char param){

//char STATUS_REG_W 0x06   //000   0011    0
//char STATUS_REG_R 0x07   //000   0011    1
//char RESET_SENSOR 0x1e   //000   1111    0

    char humi = 0;
    char temp = 1;

    char address = (param == temp)?0x03:0x05;

	char checksum;
	int result;

	//	select temp register
	IdleI2C1();
	StartI2C1();
	IdleI2C1();

	StopI2C1();
	IdleI2C1();
	RestartI2C1();
	IdleI2C1();

	if (WriteI2C1(address) == -2){
		while(1);
	}

	if (param == humi){
		__delay_ms(80);
	} else if (param == temp){
		__delay_ms(80);
		__delay_ms(80);
		__delay_ms(80);
		__delay_ms(80);
	}

	IdleI2C1();
	result=ReadI2C1()<<8;
	AckI2C1();

	IdleI2C1();
	result|=ReadI2C1();
	AckI2C1();

	IdleI2C1();
	checksum = ReadI2C1();
	NotAckI2C1();
    StopI2C();
    return result;
}

/**
 * функция перерасчета параметров из цифрового кода в % влажности/градусы
 * @param codeHumi значение влажности в цифровом коде
 * @param codeTemp значение температуры в цифровом коде
 * функция записывает значения в указатели codeHumi, codeTemp
 */
void calcSht1(float *codeHumi, float *codeTemp){

	float C1=-4.0;              // for 12 Bit
	float C2=+0.0405;           // for 12 Bit
	float C3=-0.0000028;        // for 12 Bit
	float T1=+0.01;             // for 14 Bit @ 5V
	float T2=+0.00008;           // for 14 Bit @ 5V

	float d1=-39.66;
	float d2=0.01;

    *codeTemp = d2**codeTemp+d1;
    *codeHumi = (*codeTemp-25)*(T1+T2**codeHumi)+C1+C2**codeHumi+C3**codeHumi**codeHumi;

//if(rh_true>100)rh_true=100;       //cut if the value is outside of
// if(rh_true<0.1)rh_true=0.1;       //the physical possible range
}

//значения сохраняются в data.degMaxLight, при последней итерации устанавливается data.lightPlatform.closeMeasurement. Первая итерация вкл. поворотную платформу, последняя - выключает
void mLightPlatform(void){
   	static int currentDeg = 0;

    if (data.lightPlatform.startMeasurement){//запуск
        data.lightPlatform.startMeasurement = 0;
        currentDeg = 0;
		turnPlatform(1);
    }
	if (currentDeg != data.lightPlatform.maxDeg){
		SetChanADC(ADC_CH0);
		data.lightPlatform.diod1[currentDeg/data.lightPlatform.iterationDeg] = (float)mADC()*3.3/1023;
		__delay_ms(20);

		SetChanADC(ADC_CH1);
		data.lightPlatform.diod2[currentDeg/data.lightPlatform.iterationDeg] = (float)mADC()*3.3/1023;
		__delay_ms(20);

		currentDeg += data.lightPlatform.iterationDeg;
    } else if (!data.lightPlatform.closeMeasurement){ // окончание
        data.lightPlatform.closeMeasurement = 1;
		offPlatform();
		data.degMaxLight = calcMaxDeg();
    }
}

/**
 * функция измерения освещенности
 * return возвращает измеренные значения освещенности
 */
unsigned int mLight(void){
		int result;

        IdleI2C();
        StartI2C();

        IdleI2C();
        if (WriteI2C(0x46) == -2){
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

        __delay_ms(16);//for l-resolution

        IdleI2C();
        StartI2C();

        IdleI2C();
        if (WriteI2C(0x47) == -2){
                //l-level 0x47
                //h-level 0xB9
                while(1);
        }

        IdleI2C();
        result = ReadI2C()<<8;
        AckI2C();

        IdleI2C();
        result |= ReadI2C();
        NotAckI2C();
        StopI2C();
        return (int)(result/1.2);
    }

/*
 * измерение напряжения на АЦП
 */
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
//return 1 - left, 2 - right, -1 error, 0 - free
int isTurn(void){
	if (!LATBbits.LATB4 && LATBbits.LATB2){
		return 1;
	} else if (!LATBbits.LATB2 && LATBbits.LATB4){
		return 2;
	} else if(!LATBbits.LATB2 && !LATBbits.LATB4){
		return -1;
    } else {
		return 0;
	}
}

void interrupt oneDegInterupt(void){
    int timer1Deg = 0xF937;

	if (INTCONbits.TMR0IF == 1){
	//таймер на 1 deg
		if (event.platform.on){//проводятся измерение освещенности (движ двиг)
			if (event.platform.time > 0){
				event.platform.time --;
			} else if (event.platform.time <0){
				event.platform.time ++;
			} else {

				event.platform.complete = 1;
				event.platform.on = 0;
			}
		}
		
		if (event.temp.on){
			event.temp.time--;
			if (!event.temp.time){
				event.temp.complete = 1;
				event.temp.on = 0;
			}
		}

		if (event.dump.on){
			event.dump.time--;
			if (!event.dump.time){
				//делаем включение насоса
				event.dump.complete = 1;
				event.dump.on = 0;
			}
		}

		WriteTimer0(timer1Deg);
		INTCONbits.TMR0IF = 0;
	}
}

/**
 * @param параметр, определяющий в какую сторону поворачивать платформу, если больше нуля в одну сторону, меньше - в другую
 * @return -1 - ошибка, 0 - продолжаем вращение в том-же направлелнии, 1 - влкючили поворот
 */
int turnPlatform(int direction){
//	unsigned int time = 0xA470;//3 sec
	//(4*256)/8×106 = 128uS
	//1/128uS = 7813 - 1s
    if ((!LATBbits.LATB4 && direction < 0) || (!LATBbits.LATB2 && direction > 0)){
		return -1;
    }

    if(!LATBbits.LATB2 || !LATBbits.LATB4){
        return 0;
    }

	if (direction > 0){
		LATBbits.LATB4 = 0;
	} else if (direction < 0){
		LATBbits.LATB2 = 0;
	} 

    return 1;
}

void offPlatform(){
	LATBbits.LATB4 = 1;
	LATBbits.LATB2 = 1;
}

int calcMaxDeg(void){
	int i;
	float val=0;
	int result=0;
	float sum = 0;
	for (i = 0; i < (int)(data.lightPlatform.maxDeg/data.lightPlatform.iterationDeg); i++){
		sum = data.lightPlatform.diod1[i] + data.lightPlatform.diod2[i];
		if (sum > val){
			val = sum;
			result = i;
		}
	}
	return (int)((result+1)*data.lightPlatform.iterationDeg);
}
//turn platform

//выявляем зависимость напряжения подаваемого на шим от освещенности
int calcPWM(void){
	return ~data.light;
}

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

int isDump(void){
	if (!LATBbits.LATB1){
		return 1;
	} else {
		return 0;
	}
}
//config
void configPorts(void){
	TRISB = 0;//all output
	TRISC = 0;//all output
	TRISBbits.RB3 = 1;//for pwm (1 if after timer interrupt is set 0) 0 else

//	исоплнительные механизмы
	
	//fan
	LATCbits.LATC2 = 0;//fan default off
	LATBbits.LATB0 = 0;//for ligth sensor h-level address
	//platform
	LATBbits.LATB2 = 1;//turn platform default stay
	LATBbits.LATB4 = 1;

	LATBbits.LATB5 = 1;//lamp
    LATBbits.LATB1 = 1; //pump default off
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

void configTimers(void){

	T0CONbits.TMR0ON = 1;//enables timer
	T0CONbits.T08BIT = 0;//1 - 8bit 0 - 16 bit
	T0CONbits.PSA = 0;
	//prescale settings
	T0CONbits.T0PS0 = 1;//1:256
	T0CONbits.T0PS1 = 1;
	T0CONbits.T0PS2 = 1;

	T0CONbits.T0CS = 0;//internal

//	TMR0H = 0;//reset
//	TMR0L = 0;
	INTCONbits.TMR0IE = 1;//ienable timer0 overflow interrupt
//	WriteTimer0(timer1Deg);
//	OpenTimer0();

	/*
	T1CONbits.TMR1CS0 = 0;//fosc/4
	T1CONbits.TMR1CS0 = 0;

	T1CONbits.T1CKPS0 = 1; //prescale 1:8
	T1CONbits.T1CKPS1 = 1;

	T1CONbits.T1RD16 = 1;//16bit
	T1CONbits.TMR1ON = 1;//on
	PIE1bits.TMR1IE = 1;
	INTCONbits.PEIE = 1;

//	T1CONbits.T1SYNC = 1;
//	T1CONbits.T1SOSCEN = 1;//secondary oscilator
	WriteTimer0(timer05sec);
	 */

	TMR2IF = 0;
//	T2CONbits.

	T2CONbits.TMR2ON = 1;
	//prescale

}

void configCCP(void){

    CCP2CONbits.CCP2M0 = 0;//mode pwm
    CCP2CONbits.CCP2M1 = 0;
    CCP2CONbits.CCP2M2 = 1;
    CCP2CONbits.CCP2M3 = 1;

//	PSTR2CONbits.STR2SYNC = 1;//управление происходить со след шим периодом
//	PSTR2CONbits.STR2B = 1;//используем b порт

	CCPTMRS0bits.C2TSEL0 = 0;//выбираем таймер 2
	CCPTMRS0bits.C2TSEL1 = 0;

	PR2bits.PR2 = 0x65; //установить период 19.61khz resloution 8bit

//	CCPR2Lbits.CCPR2L = 0x30; //устанавливаем длительность
//	CCPR2Lbits.CCPR2L = 0x30; //устанавливаем длительность
	CCPR2L = 0x45;
	//проверить чтобы порт был как выход rb3
}
