#ifndef __FUN_H
#define __FUN_H	 
#include "sys.h"

void fun(void);
u16 cumulative_sum(volatile u8 *Buf,u8 len);

#define TFH1 0x5c
#define RFH1 0x5C
#define TFH2 0x02
#define RFH2 0x02
#define TFH3 0x03
#define RFH3 0x03

#define SPI3FH1    0x7A
#define SPI3FH1len  200


#define  keycon    2


void interrupt(u8 bit);

void SPI3_RD(void);
void USART1_RD(void);
void USART3_RD(void);
void USART2_RD(void);
u16 Tx_summ(volatile u8 *Buf1, volatile u8 *Buf2,u16 len); //
u16 Rx_summ(volatile u8 *Buf1, volatile u8 *Buf2,u16 len); //
u16 sumc(volatile u8 *Buf1, u16 len); //ÀÛ¼ÓºÍÐ£Ñé
void key(void);
void OLEDtinct(u8 *USART,u8 *oled,u8 addr,u8 len);//½ØÈ¡OLEDµÆ¹âÏÔÊ¾
//void	memst(volatile u8 *Buf1, u16 len); //ÇåÁãÊý×é
unsigned int crc16(volatile unsigned char * puchMsg, unsigned int usDataLen);

typedef struct { 
  unsigned char BIT0: 1; 
  unsigned char BIT1: 1; 
  unsigned char BIT2: 1; 
  unsigned char BIT3: 1; 
  unsigned char BIT4: 1; 
  unsigned char BIT5: 1; 
  unsigned char BIT6: 1; 
  unsigned char BIT7: 1; 
                }TYPE_BIT; 

























#endif
