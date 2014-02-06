#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include <plib/adc.h>
#include <plib/ctmu.h>

#define USE_OR_MASKS

void main(void) {

//    unsigned char config1 = 0, config2 = 0, config3 = 0;
    float adccount = 0;

//	config1 = ADC_LEFT_JUST;
//	config2 = ADC_CH16;
//	config3 = ADC_REF_VDD_VDD & ADC_REF_VDD_VSS;

//	OpenADC(config1,config2,config3);
    ADCON0 = 0b00000001;
    ADCON1 = 0b00001111;
    ADCON2 = 0b10001000;
    

    while(1){

		ADRESH=0;//clear the ADC result register
		ADRESL=0;//clear the ADC result register

		/* Read ADC*/
		ConvertADC(); // stop sampling and starts adc conversion
		while(BusyADC()); //wait untill the conversion is completed
		adccount = 3.3*(float)ReadADC()/1023;//read the result of conversion
    }
	
}