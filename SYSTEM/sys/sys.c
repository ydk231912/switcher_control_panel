#include "sys.h"

//********************************************************************************  
void NVIC_Configuration(void)
{

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);	//����NVIC�жϷ���2:2λ��ռ���ȼ���2λ��Ӧ���ȼ�

}


/**************************************************************************
Function: Set JTAG mode
Input   : mode:JTAG, swd mode settings;00,all enable;01,enable SWD;10,Full shutdown
Output  : none
????:??JTAG??
????:mode:jtag,swd????;00,???;01,??SWD;10,???;	
??  ?:?
**************************************************************************/
//#define JTAG_SWD_DISABLE   0X02
//#define SWD_ENABLE         0X01
//#define JTAG_SWD_ENABLE    0X00	
void JTAG_Set(u8 mode)
{
	u32 temp;
	temp=mode;
	temp<<=25;
	RCC->APB2ENR|=1<<0;     //??????	   
	AFIO->MAPR&=0XF8FFFFFF; //??MAPR?[26:24]
	AFIO->MAPR|=temp;       //??jtag??
} 
//��ʼ���������Ź�
//prer:��Ƶ��:0~7(ֻ�е�3λ��Ч!)
//��Ƶ����=4*2^prer.�����ֵֻ����256!
//rlr:��װ�ؼĴ���ֵ:��11λ��Ч.
//ʱ�����(���):Tout=((4*2^prer)*rlr)/40 (ms).
void IWDG_Init(u8 prer,u16 rlr) 
{
	  IWDG->KR=0X5555;//ʹ�ܶ�IWDG->PR��IWDG->RLR��д		 										  
  	IWDG->PR=prer;  //���÷�Ƶϵ��   
  	IWDG->RLR=rlr;  //�Ӽ��ؼĴ��� IWDG->RLR  
	  IWDG->KR=0XAAAA;//reload											   
  	IWDG->KR=0XCCCC;//ʹ�ܿ��Ź�	
}
//ι�������Ź�
void IWDG_Feed(void)
{
	IWDG->KR=0XAAAA;//reload											   
}
