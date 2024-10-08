#ifndef __TIM_H
#define __TIM_H
#include "sys.h"


void TIM3_Int_Init(u16 arr,u16 psc);
void TIM2_Int_Init(u16 arr,u16 psc);

extern u16 dma_tim; 

extern u16 USART3tim;
extern u16 usart1time1,usart2time1,usart3time1;   //定时发送
extern u16 usart1time2,usart2time2,usart3time2;//定时发送  //定时发送

extern u16 SPItim; 
extern u16 cantim;
extern u16 rgbwtim;

#endif

