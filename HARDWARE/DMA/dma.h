#ifndef __DMA_H
#define	__DMA_H	   
#include "sys.h"

#define U1orU2  0

#if U1orU2

#define		  DMARX1C     724
#define		  DMARX2C			173 
#define     cantimset   7 
#else

#define		  DMARX1C     244
#define		  DMARX2C			76
#define     cantimset   5
#endif





#define		  DMARX2			180
#define		  DMARX1			750

#define 	  DMATX1			30			 //DMA传输发送字节数
#define 	  DMATX2			10			 //DMA传输发送字节数
#define 	  DMATX3			12		   //DMA传输发送字节数
#define		  DMARX3			12	
#define     DMATX1C     30
#define 	  DMATX2C			15			 //DMA传输发送字节数
#define 	  DMATX3C			10			 //DMA传输发送字节数
#define		  DMARX3C			10

extern volatile u8  ucByteBufRX1[DMARX1];
extern volatile u8  ucByteBufTX1[DMATX1];
extern volatile u8  ucByteBufRX2[DMARX2];    //串口1通讯数组
extern volatile u8  ucByteBufTX2[DMATX2];

extern volatile u8  ucByteBufRX3[DMARX3];    //串口1通讯数组
extern volatile u8  ucByteBufTX3[DMATX3];




void DMA_USART1_Tx_Config(DMA_Channel_TypeDef*DMA_CHx,u32 cpar,u32 cmar,u16 cndtr);//配置DMA1_CHx
void DMA_USART1_Rx_Config(DMA_Channel_TypeDef*DMA_CHx,u32 cpar,u32 cmar,u16 cndtr);//配置DMA1_CHx

void DMA_USART2_Tx_Config(DMA_Channel_TypeDef*DMA_CHx,u32 cpar,u32 cmar,u16 cndtr);//配置DMA1_CHx
void DMA_USART2_Rx_Config(DMA_Channel_TypeDef*DMA_CHx,u32 cpar,u32 cmar,u16 cndtr);//配置DMA1_CHx

void DMA_USART1_Tx(DMA_Channel_TypeDef*DMA_CHx,u16 len);//使能DMA1_CHx
void DMA_USART2_Tx(DMA_Channel_TypeDef*DMA_CHx,u16 len);//使能DMA1_CHx		 
void DMA_USART3_Tx(DMA_Channel_TypeDef*DMA_CHx,u16 len);//使能DMA1_CHx		


void DMA_USART3_Tx_Config(DMA_Channel_TypeDef*DMA_CHx,u32 cpar,u32 cmar,u16 cndtr);//配置DMA1_CHx
void DMA_USART3_Rx_Config(DMA_Channel_TypeDef*DMA_CHx,u32 cpar,u32 cmar,u16 cndtr);//配置DMA1_CHx


void DMA_SPI2_Tx_Config(void);
void DMA_SPI2_Tx(u8 *buff,u16 LEN);





#endif




