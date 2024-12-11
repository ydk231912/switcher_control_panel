#ifndef __FUN_H
#define __FUN_H	 
#include "sys.h"

void fun(void);
u16 cumulative_sum(volatile u8 *Buf,u8 len);
#define TFH1 0x03
#define RFH1 0x03

#define TFH2 0x02
#define RFH2 0x02

#define TFH3 0x03
#define RFH3 0x03




#define  keycon    2    //按键延时
#define  kcon      2    //按键延时


void  interrupt(u8 bit);
void  delayy(u16 i);
void  SPI1_RD(void);
void  USART3_RD(void);
void  USART1_RD(void);
void USART2_RD(void);
void key(void);
void address_selection(void);
u16 Rx_summ(volatile u8 *Buf1, volatile u8 *Buf2,u16 len); //ÀÛ¼ÓºÍÐ£Ñé
u16 sumc(volatile u8 *Buf1, u16 len); //ÀÛ¼ÓºÍÐ£Ñé
void OLEDtinct(u8 *USART,u8 *oled,u8 addr,u8 len); //截取OLED数据
//void memst(volatile u8 *Buf1, u16 len); //
unsigned int crc16(volatile unsigned char * puchMsg, unsigned int usDataLen);

extern u8 SPIon;
extern u8 oledon;  //¿ªÆôOLEDÆÁ

#define oledlen   46
extern u8 oled1[oledlen];
extern u8 oled2[oledlen];
extern u8 oled3[oledlen];
extern u8 oled4[oledlen];
extern u8 oled5[oledlen];
extern u8 oled6[oledlen];

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


#define Key1  rM8xxx[0].BIT.BIT0



void oleddata(u8*Rx1Beye,u8*oledbeye, u16 addr);

#endif
