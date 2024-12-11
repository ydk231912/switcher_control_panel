#ifndef __USART_H
#define __USART_H
#include "stdio.h"	
#include "sys.h" 

////////////////////////////////////////////////////////////////////////////////// 	


extern u16 USART1_RxLen;   //���ڽ��յ�����������
extern u16 USART2_RxLen;
void uart1_init(u32 pclk2,u32 bound);
void uart2_init(u32 pclk2,u32 bound);
void uart3_init(u32 pclk2,u32 bound);
//void uart2_init2(u32 pclk2,u32 bound);
extern u8 USART1_RxOV,USART1_TxOV; //����1������ɱ�־
extern u8 USART2_RxOV,USART2_TxOV; //����2������ɱ�־
extern u8 USART3_RxOV,USART3_TxOV; //����2������ɱ�־
#endif


