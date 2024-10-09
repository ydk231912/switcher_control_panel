#ifndef __ADC__H__
#define __ADC__H__	

#include "sys.h"



void  Adc_Init(void);
void  Get_Adc_Average1(u16 times);

extern s16 ADC1_X,ADC1_Y,ADC2_A,ADC2_B;
extern u16 reference_V;  //µ±«∞µÁ—π
void ADC_zero_calibration(void);

#endif



