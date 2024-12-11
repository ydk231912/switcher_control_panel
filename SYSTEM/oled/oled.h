#ifndef __OLED_H
#define __OLED_H 

#include "sys.h"

#define USE_HORIZONTAL 0  //������ʾ���� 0��������ʾ��1����ת180����ʾ


//SCL=SCLK 
//SDA=MOSI
//RES=RES
//DC=DC
//CS=CS
//-----------------OLED�˿ڶ���---------------- 

//#define OLED_SCL_Clr() GPIO_ResetBits(GPIOB,GPIO_Pin_3)//SCL
//#define OLED_SCL_Set() GPIO_SetBits(GPIOB,GPIO_Pin_3)

//#define OLED_SDA_Clr() GPIO_ResetBits(GPIOB,GPIO_Pin_5)//SDA
//#define OLED_SDA_Set() GPIO_SetBits(GPIOB,GPIO_Pin_5)

//#define OLED_RES_Clr()  GPIO_ResetBits(GPIOD,GPIO_Pin_8)//RES
//#define OLED_RES_Set()  GPIO_SetBits(GPIOD,GPIO_Pin_8)

//#define OLED_DC_Clr()  GPIO_ResetBits(GPIOD,GPIO_Pin_9)//DS
//#define OLED_DC_Set()  GPIO_SetBits(GPIOD,GPIO_Pin_9)
 		     
//#define OLED_CS_Clr()  GPIO_ResetBits(GPIOE,GPIO_Pin_15)//CS
//#define OLED_CS_Set()  GPIO_SetBits(GPIOE,GPIO_Pin_15)


#define   SSCL   PBout(3)
#define   SSDA   PBout(5) 

#define   CS1    PEout(15)
#define   RES1   PDout(8) 
#define   DC1    PDout(9)

#define   CS2    PCout(12)
#define   RES2   PCout(10) 
#define   DC2    PCout(11)

#define   CS3    PDout(0)
#define   RES3   PDout(2) 
#define   DC3    PDout(1)



//��ʾ�����ܺ���
void OLED_WR_REG(u8 reg);//д��һ��ָ��
void OLED_WR_Byte(u8 dat);//д��һ������
void Column_Address(u8 a,u8 b);//�����е�ַ
void Row_Address(u8 a,u8 b);//�����е�ַ
void OLED_Fill(u16 xstr,u16 ystr,u16 xend,u16 yend,u8 color);//��亯��

void OLED_ShowChar(u8 x,u8 y,u8 chr,u8 sizey,u8 mode);//��ʾ�ַ�
void OLED_ShowString(u8 x,u8 y,u8 *dp,u8 sizey,u8 mode);//��ʾ�ַ���
u32 oled_pow(u8 m,u8 n);//�ݺ���
void OLED_ShowNum(u8 x,u8 y,u32 num,u8 len,u8 sizey,u8 mode);//��ʾ��������
void OLED_Init(void);




void initialize(void);

void clear(void);
extern u8 CSse; //��ʾ��ѡ��


#endif







