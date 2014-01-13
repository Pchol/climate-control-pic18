#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include <plib/adc.h>
#include <delays.h>
#include <plib/ctmu.h>

#define USE_OR_MASKS

float voltage,current,capacitance,time;

void main(void) {

    unsigned char ctmucon1=0,ctmucon2=0,ctmuicon=0,config1=0,config2=0,config3=0,i=0;
	unsigned int adccount=0;

	// current = 0.000055 ; // 55uA - 100_BASE_CURR
	//current = 0.0000055 ; // 5.5uA - 10_BASE_CURR
	current = 0.00000055 ; // 0.55uA - BASE_CURR
	TRISA= TRISA | 0x0001; //Configure RA1 as input pin

	/*Configure ADC to read channel 1*/
	//---initialize adc---
	/**** ADC configured for:
	* FOSC-RC as source of conversion clock
	* Result is right justified
	* Aquisition time of 2 AD
	* Channel 7 for sampling
	* ADC interrupt off
	* ADC reference voltage from VDD & VSS
	*/

    //main page for pic18f45k22 family 879
    //for pic18f45k22 family using config on 929 page
	config1 = ADC_FOSC_RC | ADC_RIGHT_JUST | ADC_2_TAD ;
	config2 = ADC_CH0 | ADC_INT_OFF;
	config3 = ADC_TRIG_CTMU | ADC_REF_VDD_VREFPLUS | ADC_REF_VDD_VSS;

	OpenADC(config1,config2,config3);
	ADRESH=0;//clear the ADC result register
	
	ADRESL=0;//clear the ADC result register
	
    //ctmu for 18f45k22 page 925
    /*Configure the CTMU*/
	//---------------------------------------------------------------------
	/***** CTMU configured for:
	* Edge 1 programmed for a positive edge response
	* Edge 2 programmed for a positive edge response
	* CTED1 is a source select for Edge
	* trigger output disaled
	* Edge sequence of CTMU disabled
	* no edge delay generation
	* CTMU edges blocked
	* Current of 0.55uA
	*/

    ctmucon1 = CTMU_ENABLE | CTMU_IDLE_STOP | CTMU_TIME_GEN_ENABLE | CTMU_EDGE_SEQUENCE_OFF | CTMU_ANA_CURR_SOURCE_GND | CTMU_TRIG_OUTPUT_DISABLE | CTMU_INT_OFF;
    ctmucon2 = CTMU_EDGE2_POLARITY_POS | CTMU_EDGE2_SOURCE_CTED1 | CTMU_EDGE1_POLARITY_POS | CTMU_EDGE1_SOURCE_CTED1;
    ctmuicon = 0;

//	ctmucon2 = CTMU_EDGE1_POLARITY_POS | CTMU_EDGE2_POLARITY_POS | CTMU_EDGE1_SOURCE_CTED1 | CTMU_EDGE2_SOURCE_CTED1 ;
//	ctmucon1 = CTMU_TRIG_OUTPUT_DISABLE | CTMU_EDGE_SEQUENCE_OFF | CTMU_TIME_GEN_DISABLE | CTMU_EDGE_DISABLE ;

	OpenCTMU(ctmucon1,ctmucon2,ctmuicon);

    Enbl_CTMUEdge1;//Enable current source

	/* Wait for 50 usec*/
	Delay10TCYx(0x05);

	Disbl_CTMUEdge1; //Disable current source

	PIR1bits.ADIF=0; //clear the ADC interrupt

	/* Read ADC*/
	ConvertADC(); // stop sampling and starts adc conversion
	while(BusyADC()); //wait untill the conversion is completed
	adccount = ReadADC();//read the result of conversion
	
	/* Capacitance calculation */
	time = 0.00005;

	voltage = (adccount*3.3)/1024; // convert ADC count into voltage
	capacitance = (current * time)/voltage; // calculate the Capacitance value
	CloseADC();// disable ADC
	CloseCTMU();//disable CTMU

	while(1);//End of program
}