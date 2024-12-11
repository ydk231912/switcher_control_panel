#ifndef __TIM_H
#define __TIM_H
#include "sys.h"


void TIM3_Int_Init(u16 arr,u16 psc);
void TIM2_Int_Init(u16 arr,u16 psc);

extern u16 dma_tim; 

extern u16 ADtim;
extern u16 cantim;
extern u16 rgbwtim;
extern u16 OLEDtim; 
#endif

