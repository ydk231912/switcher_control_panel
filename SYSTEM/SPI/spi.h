#ifndef __SPI_H
#define __SPI_H	 
#include "sys.h"
#include "stm32f10x.h"

#define SCLK  PBout(12)  //ʱ��
#define SO    PBout(13)  //�������
#define CS    PBout(14)  //
#define SI    PBin(15)   //��������





//void SPI_RWmcu(u8 *TxBuf,u8 *RxBuf,u8 len);	
//void SPI_Read(u8 *TxBuf,u8 *RxBuf,u8 len);	
//u8 SPI_RW(u8 buchar);	
//void SPI_RWmcu1(u8 buchar);	

void SPI_Configuration(void);
void SPI1_Send_Byte(unsigned char dat);
void delay(u16 m);


void SPI1_Init(void);			 //��ʼ��SPI��
void SPI1_ReadorWriteByte(u8 *TxData,u8 *RxData,u16 len);


void SPI2_Init(void);			 //��ʼ��SPI��
u8 SPI2_ReadWriteByte(u8 TxData);//SPI���߶�дһ���ֽ�
u8 SPI2_WriteByte(u8 * buff,u16 len);



void SPI3_Init(void);
u8 SPI3_ReadWriteByte(u8 TxData);
u8 SPI3_WriteByte(u8 * buff,u16 len);

u8 SPI1_ReadWriteByte(u8 TxData);//��������
#endif

