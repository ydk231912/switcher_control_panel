#ifndef __USART_H
#define __USART_H
#include "stdio.h"	
#include "sys.h" 

////////////////////////////////////////////////////////////////////////////////// 	


extern u16 USART1_RxLen;   //串口1接收到的数据数量
extern u16 USART2_RxLen;   //串口1接收到的数据数量
void uart1_init(u32 pclk2,u32 bound);
void uart2_init(u32 pclk2,u32 bound);
void uart3_init(u32 pclk2,u32 bound);
//void uart2_init2(u32 pclk2,u32 bound);
extern u8 USART1_RxOV,USART1_TxOV; //串口1接收完成标志
extern u8 USART2_RxOV,USART2_TxOV; //串口2接收完成标志
extern u8 USART3_RxOV,USART3_TxOV; //串口3接收完成标志
#endif


