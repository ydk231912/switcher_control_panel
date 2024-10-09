#include "sys.h"

//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK Mini STM32开发板
//系统中断分组设置化		   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//修改日期:2012/9/10
//版本：V1.4
//版权所有，盗版必究。
//Copyright(C) 正点原子 2009-2019
//All rights reserved
//********************************************************************************  
void NVIC_Configuration(void)
{

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);	//设置NVIC中断分组2:2位抢占优先级，2位响应优先级

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
