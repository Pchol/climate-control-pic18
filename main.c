#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include <plib/adc.h>
#include <plib/ctmu.h>

#define USE_OR_MASKS

void main(void) {

    unsigned char config1 = 0, config2 = 0, config3 = 0;
    unsigned int adccount = 0, i=0;

	config1 = ADC_LEFT_JUST;
	config2 = ADC_CH16;
	config3 = ADC_REF_VDD_VDD & ADC_REF_VDD_VSS;

	OpenADC(config1,config2,config3);

	ADRESH=0;//clear the ADC result register
	ADRESL=0;//clear the ADC result register

	/* Read ADC*/
	ConvertADC(); // stop sampling and starts adc conversion
	while(BusyADC()); //wait untill the conversion is completed
	adccount = ReadADC();//read the result of conversion
	
	while(1);//End of program
}