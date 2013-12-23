#include "p18f25k22.h"
#include "config.h"
#include <stdlib.h>


//#include <delays.h>

/**************************************************************************/
/*Set up CTMU *****************************************************************/
/**************************************************************************/
void setup(void)
{
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

unsigned int measure(void){
		PIR1bits.ADIF = 0; //make sure A/D Int not set
		ADCON0bits.GO=1; //and begin A/D conv.
		while(!PIR1bits.ADIF); //Wait for A/D convert complete
		
		PIR1bits.ADIF = 0; //Clear A/D Interrupt Flag
		return ADRES; //Get the value from the A/D
}

void main(void){
	unsigned int Vread=0;
	float voltage;// Vcal=0, CTMUISrc = 0;
    float capacitance=0.0;
	static float time = 30.0;
    float current = 0.55;//experiment time*current
//	float ownCap =;//own capacitance
	float v = 3.3;

	OSCCONbits.IRCF2 = 1;
	OSCCONbits.IRCF1 = 1;
	OSCCONbits.IRCF0 = 0;

	setup();

    while(1){
		CTMUCONHbits.CTMUEN = 1; //Enable the CTMU
		CTMUCONLbits.EDG1STAT = 0; // Set Edge status bits to zero
		CTMUCONLbits.EDG2STAT = 0;
		CTMUCONHbits.IDISSEN = 1; //drain charge on the circuit
		__delay_us(30);
		CTMUCONHbits.IDISSEN = 0; //end drain of circuit

		CTMUCONLbits.EDG1STAT = 1;//Begin charging the circuit
		__delay_us(30);
		CTMUCONLbits.EDG1STAT = 0; 	 //Stop charging circuit
		Vread = measure();

		voltage = (float)(v*Vread/1023.0);
		capacitance = (float)(time*current/voltage);//result in pf
    }
}

