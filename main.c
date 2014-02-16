#include "config.h"
#include <plib/adc.h>
//#include <plib/ctmu.h>
#include <plib/i2c.h>
#include <plib/timers.h>
#include <plib/pwm.h>
//индикация
//A6 - насос
//A5 - лампа
//A4 - ветнилятор
//C1 - светодиоды

#define USE_OR_MASKS

    struct {
        float temp;
        float humi;
        float capacitance;
        unsigned int light;
        int degMaxLight;

        struct {
            int maxDeg;//значения maxDeg обязательно должно быть кратно iterationDeg
            int iterationDeg;
            int status;//0-измерения не проводились, 1 проводится измерение, 2 возврат на нужн. угол, 3 остановка, 4 конец
            int currentDeg;//текущий угол измерения
            float diod1[36];
            float diod2[36];
        } lightPlatform;
    } data = {0, 0, 0, 0, 0, {360, 10, 0}};

    struct {
        char maxTemp;
        char minTemp;
        char maxHumi;
        char minHumi;
        char minCapacitance;
    } level = {33, 30, 34, 32, 30};

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
    float mC(void);
    void mLightDiods(float *rightDiod, float *leftDiod);
    
    int calcPWM(void);
    int calcMaxDeg(void);
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
    void configCTMU(void);

    int isLamp(void);
    int isTurn(void);
    int isFan(void);
    int isDump(void);

    int turnPlatform(int move);
    void offPlatform(void);

    int secToDeg(int sec);

    void main(void) {

          init();

          while(1){

              if (event.platform.complete){

                  if (data.lightPlatform.status == 0){//старт
                      data.lightPlatform.currentDeg = 0;
                      turnPlatform(1);
                      mLightDiods(&data.lightPlatform.diod1[0], &data.lightPlatform.diod2[0]);
                      data.lightPlatform.status = 1;
                      event.platform.time = data.lightPlatform.iterationDeg;

                  } else if (data.lightPlatform.status == 1){//измерения

                      if (data.lightPlatform.currentDeg != data.lightPlatform.maxDeg){
                          int i = data.lightPlatform.currentDeg/data.lightPlatform.maxDeg;
                          mLightDiods(&data.lightPlatform.diod1[i], &data.lightPlatform.diod2[i]);
                          data.lightPlatform.currentDeg += data.lightPlatform.iterationDeg;
                          event.platform.time = data.lightPlatform.iterationDeg;
                      } else {
                          offPlatform();
                          data.degMaxLight = calcMaxDeg();
                          data.lightPlatform.status = 2;
                          event.platform.time = secToDeg(1);
                      }

                } else if (data.lightPlatform.status == 2){//возврат на нужный угол
                        int returnDeg = -(data.lightPlatform.maxDeg - data.degMaxLight);
                        if (returnDeg < 0){
                            turnPlatform(-1);
                            event.platform.time = returnDeg;
                            data.lightPlatform.status = 3;
                        } else {
                            data.lightPlatform.status = 4;
                            event.platform.time = 1;
                        }
                        
                } else if (data.lightPlatform.status == 3){//остановка
                    offPlatform();
                    data.lightPlatform.status = 4;
                    event.platform.time = 2;

				  event.temp.time = 1;
				  event.temp.on = 1;

                }

                event.platform.complete = 0;
                if (data.lightPlatform.status != 4){
                    event.platform.on = 1;
                }
            }

            if (event.dump.complete){
                if (!isDump()){
                    data.capacitance = mC();
                    if (data.capacitance < level.minCapacitance){
                        LATBbits.LATB1 = 0;
						LATAbits.LATA6 = 1;//indication
                        event.dump.time = secToDeg(5);//длительность работы насоса
                    } else {
                        event.dump.time = secToDeg(30);//время между измерениями
                    }
                    event.dump.on = 1;
                } else if (isDump()){
                    LATBbits.LATB1 = 1;
					LATAbits.LATA6 = 0;//indication

                    event.dump.time = secToDeg(60);//1min ждем, пока вода опустится до дна
                    event.dump.on = 1;
                }
                    event.dump.complete = 0;
            }

            if (event.temp.complete){
                    
                measurement();//производим измерения со всех датчков
                compare();//сравниваем значения с уровнями
                execution();//запускаем устройства

                event.temp.time = 1;
                event.temp.complete = 0;
                event.temp.on = 1;
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
      configCTMU();

      while(!TMR2IF);
      TRISBbits.RB3 = 0;
      
//      event.dump.time = 1;
//      event.dump.on = 1;
//      event.temp.time = 1;
//      event.temp.on = 1;
      event.platform.time = 1;
      event.platform.on = 1;
  }

  /**
   * измерения
   */
  void measurement(void){
      data.temp = mSht1(1);//изм. темпр.
      data.humi = mSht1(0);//изм. влажность
      calcSht1(&data.temp, &data.humi);//перерасчет

      data.light = mLight();//освещенность
  }

  /**
   * сравнение значений с уровнями
   */
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

  /**
   * запуск устройства
   */
  void execution(void){
      if (run.lampOn){
          LATBbits.LATB5 = 0;
		  LATAbits.LATA5 = 1;
          run.lampOn = 0;
      } else if (run.lampOff){
          LATBbits.LATB5 = 1;
		  LATAbits.LATA5 = 0;
          run.lampOff = 0;
      }

      if (run.fanOn){
          LATCbits.LATC2 = 1;
		  LATAbits.LATA4 = 1;
          run.fanOn = 0;
      } else if (run.fanOff){
		  LATAbits.LATA4 = 0;
          LATCbits.LATC2 = 0;
          run.fanOff = 0;
      }

	  CCPR2L = calcPWM();
//      SetDCEPWM2(calcPWM());
  }

  /**
   * измерение емкости
   * измеренные значения емкости
   */
  float mC(void){
      unsigned int Vread=0;
      float voltage;// Vcal=0, CTMUISrc = 0;
      float time = 15;
      float current = 0.55;//experiment time*current
      float ownCap = 33.5;//own capacitance
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
      return (float)(time*current/voltage - ownCap);//result in pf
  }

  /**
   * измерение влажности или температуры воздуха
   * char param - параметр, определяющий что будем измерять (0 - влажность, 1 - температура)
   * измеренные значения сохраняются в data.codeTemp или data.codeHumi
   * return возвращает измеренные значения
   */
  int mSht1(char param){

      char humi = 0;
      char temp = 1;
      char address = (param == temp)?0x03:0x05;
      char checksum;
      int result;

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
   * перерасчет параметров из цифрового кода в % влажности/градусы
   * @param codeHumi значение влажности в цифровом коде
   * @param codeTemp значение температуры в цифровом коде
   * функция записывает значения в указатели codeHumi, codeTemp
   */
  void calcSht1(float *codeTemp, float *codeHumi){
	  float t = *codeTemp;
	  float ch = *codeHumi;

      float C1=-4.0;              // for 12 Bit
      float C2=+0.0405;           // for 12 Bit
      float C3=-0.0000028;        // for 12 Bit
      float T1=+0.01;             // for 14 Bit @ 5V
      float T2=+0.00008;           // for 14 Bit @ 5V

      float d1=-39.66;
      float d2=0.01;

	  t=d2*t+d1;
	  ch=(t-25)*(T1+T2*ch)+C1+C2*ch+C3*ch*ch;

//	  *codeTemp=d2*(*codeTemp)+d1;
//      *codeHumi=(*codeTemp-25)*(T1+T2**codeHumi)+C1+C2**codeHumi+C3**codeHumi**codeHumi;

	  *codeTemp = t;
	  *codeHumi = ch;

      //if(rh_true>100)rh_true=100;       //cut if the value is outside of
      // if(rh_true<0.1)rh_true=0.1;       //the physical possible range
  }

  /**
  * считываем значения с диодов
  */
  void mLightDiods(float *rightDiod, float *leftDiod){
      SetChanADC(ADC_CH0);
      *rightDiod = (float)mADC()*3.3/1023;
      __delay_ms(20);

      SetChanADC(ADC_CH1);
      *leftDiod = (float)mADC()*3.3/1023;
  }

  /**
   * измерение освещенности
   * @return измеренные значения освещенности
   */
  unsigned int mLight(void){
      int result;

      IdleI2C();
      StartI2C();

      IdleI2C();
      if (WriteI2C(0x46) == -2){//l-level 0xB8 //h-level 0x46
        while(1);
      }

      IdleI2C();
      if (WriteI2C(0x13) == -2){//0x10 h-res  //0x13 l-res
                      while(1);
      }

      IdleI2C();
      StopI2C();

      __delay_ms(16);//for l-resolution

      IdleI2C();
      StartI2C();

      IdleI2C();
      if (WriteI2C(0x47) == -2){//l-level 0x47  //h-level 0xB9
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

  /**
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

  /**
   * обработчик прерывания
  */
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
 * @param move параметр, определяющий в какую сторону поворачивать платформу, если больше нуля вправo, меньше - влево
 * @return -1 - ошибка, меньше 0 поворот вправо, больше нуля поворот влево
 */
int turnPlatform(int move){
    if ((!LATBbits.LATB4 && move < 0) || (!LATBbits.LATB2 && move > 0)){
        return -1;
    }
    if(!LATBbits.LATB2 || !LATBbits.LATB4){
        return 0;
    }
    if (move > 0){
        LATBbits.LATB4 = 0;
    } else if (move < 0){
        LATBbits.LATB2 = 0;
    } 
    return 1;
}

/**
 * выключаем платформу
 */
void offPlatform(){
    LATBbits.LATB4 = 1;
    LATBbits.LATB2 = 1;
}

/**
 * расчет максимального угла освещенности
 */
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

//зависимость напряжения подаваемого на шим от освещенности
int calcPWM(void){
    //0x65 - max;
	/*
	if (data.light > 100){
		return 0x65;
	} else {
		return 0;
	}*/
	static int i = 0x15;

    if (data.light > 1500){
		LATCbits.LATC1 = 0;
        return 0;
    } else {
		LATCbits.LATC1 = 1;
		i++;
		i++;
		if (i>0x65){
			i = 0x15;
		}
		return (int)(i);
    }
}

/**
 * проверяем состояни поворотной платформы
 * return 1 - вращается вправо, 2 - влево, - 1 ошибка, 0 - платформа выключена
 */
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

/**
 * проверка состояния лампы
 * @return 1 - лампа включена, 0 - лампа выключена
 */
int isLamp(void){
    if (!LATBbits.LATB5){
            return 1;
    } else {
            return 0;
    }
}

/**
 * проверка состояния вентилятора
 * @return 1 - вентилятора включен, 0 - вентилятор выключен
 */
int isFan(void){
    if (LATCbits.LATC2){
            return 1;
    } else {
            return 0;
    }
}

/**
 * проверка состояния насоса 
 * @return 1 - насос включен, 0 - насос выключен 
 */
int isDump(void){
    if (!LATBbits.LATB1){
            return 1;
    } else {
            return 0;
    }
}

/**
 * конфигурирование портов
 */
void configPorts(void){
    TRISB = 0;//all output
    TRISC = 0;//all output
	//diods
	TRISAbits.RA4 = 0;
	TRISAbits.RA5 = 0;
	TRISAbits.RA6 = 0;
	TRISCbits.RC1 = 0;

	LATAbits.LATA4 = 0;
	LATAbits.LATA5 = 0;
	LATAbits.LATA6 = 0;
	LATCbits.LATC1 = 0;

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

/**
 * конфигурирования I2C
 */
void configI2C(void){

    TRISC |= 0x18;//input rc3 and rc4
    SSP1ADD = 0xf9;					// 100KHz (Fosc = 4MHz)
    ANSELC = 0x0;					// No analog inputs req'

    OpenI2C1(MASTER, SLEW_OFF);
    DisableIntI2C1;
}

/**
 * конфигурирование CTMU
 */
void configCTMU(void){

    CTMUCONH = 0x00;
    CTMUCONL = 0x90;
    CTMUICON = 0x01;//0.55uA, Nominal - No Adjustment
    TRISA=0x04;
    ANSELAbits.ANSA2=1;//set channel 2 as an input
    TRISAbits.TRISA2=1;  // Configure AN2 as an analog channel
    ADCON2bits.ADFM=1;
    ADCON2bits.ACQT=1;
    ADCON2bits.ADCS=2;
    ADCON1bits.PVCFG0 =0;
    ADCON1bits.NVCFG1 =0;
    ADCON0bits.CHS=2;
    ADCON0bits.ADON=1;
}

/**
 * конфигурирование модуля АЦП
 */
void configADC(void){

    TRISA = 0x03;
    ADCON0 = 0b00000001;
    ADCON1 = 0b00001111;
    ADCON2 = 0b10001000;
}

/**
 * конфигурирование таймеров
 */
void configTimers(void){

    T0CONbits.TMR0ON = 1;//enables timer
    T0CONbits.T08BIT = 0;//1 - 8bit 0 - 16 bit
    T0CONbits.PSA = 0;
    T0CONbits.T0PS0 = 1;////prescale settings 1:256
    T0CONbits.T0PS1 = 1;
    T0CONbits.T0PS2 = 1;
    T0CONbits.T0CS = 0;//internal
    INTCONbits.TMR0IE = 1;//ienable timer0 overflow interrupt

    TMR2IF = 0;
    T2CONbits.TMR2ON = 1;//prescale
}

/**
 * конфигурирование модуля ccp
 */
void configCCP(void){

    CCP2CONbits.CCP2M0 = 0;//mode pwm
    CCP2CONbits.CCP2M1 = 0;
    CCP2CONbits.CCP2M2 = 1;
    CCP2CONbits.CCP2M3 = 1;
//	PSTR2CONbits.STR2SYNC = 1;//управление происходить со след шим периодом
    CCPTMRS0bits.C2TSEL0 = 0;//выбираем таймер 2
    CCPTMRS0bits.C2TSEL1 = 0;
    PR2bits.PR2 = 0x65; //установить период 19.61khz resloution 8bit

    CCPR2L = 0x0;//устанавливаем длительность
}

/**
 * функция преобразования секунд в углы (значения приблизитеьные)
 * @return значение углов
 */
int secToDeg(int sec){
    return (int)(4.5*(float)sec);//угол равен 1/4.5 с
}
