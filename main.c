#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include <plib/adc.h>
#include <delays.h>
#include <plib/ctmu.h>

/*
 * 
 */
#define USE_OR_MASKS

void sleep1s(){
int i;
//	for (i=0;i<100;i++){
		__delay_ms(9);
//    }
}

float valtage, current, capacitance, time;

int measuringADC(){
	unsigned int result;
//	ADC_INT_ENABLE();
	ConvertADC();//start

	while(BusyADC());
	result = ReadADC();
	return result;
}

int main(int argc, char** argv) {
	unsigned int adc;

	//configuration ADC module
	OpenADC(ADC_FOSC_2, ADC_CH_CTMU & ADC_INT_OFF, ADC_TRIG_CTMU);

	//configuration CTMU module
	OpenCTMU(CTMU_ENABLE, CTMU_IDLE_STOP, CTMU_EDGE2_POLARITY_POS);

	Enbl_CTMUEdge1;//enable current source

	/* Wait 50 usec */
	Delay10TCYx(0x05);

	Disbl_CTMUEdge1;

	adc = measuringADC();

	CloseADC();
	CloseCTMU();

	sleep1s();
    return (EXIT_SUCCESS);
}

