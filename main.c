#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include <plib/adc.h>
#include <plib/ctmu.h>

#define USE_OR_MASKS

float valtage, current, capacitance, time;

int measuringADC(){
	unsigned int result;
//	ADC_INT_ENABLE();
	ConvertADC();//start

	while(BusyADC());
	result = ReadADC();
	return result;
}

//main
int main(int argc, char** argv) {
    init();

    while(1){
        measurement();
        compare();
        execution();
    }
}

struct data {
    float adc;
    float humidutyWather;
    float humidutySoil;
};

struct level {

};

void measurement(){
   //измерение влажности почвы
   //измерение влажности воздуха и температуры воздуха

}

void compare(){
    //сравнение с показаниями и принятие решение какие устройства должны быть включены
}

void execution(){
    //исполнение
}

//function initializiation all components
void init(){
    configADC();
    configCTMU();

}