#ifndef __DMA_H
#define	__DMA_H	   
#include "sys.h"


#define 	  DMATX1			390			 //DMA传输发送字节数
#define		  DMARX1			390

#define 	  DMATX2			175			 //DMA传输发送字节数
#define		  DMARX2			175


#define 	  DMATX3			390			 //DMA传输发送字节数
#define		  DMARX3			390


#define     DMATX1C     387
#define		  DMARX1C     388

#define 	  DMATX2C			171			 //DMA传输发送字节数
#define		  DMARX2C			173

#define 	  DMATX3C			387		 //DMA传输发送字节数
#define		  DMARX3C			387

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




