#include "delay.h"
#include "sys.h"

static u8  fac_us=0;		//us延时倍乘数

//=====================================================================================
//如果使用ucos,则包括下面的头文件即可.
#if SYSTEM_SUPPORT_UCOS
#include "includes.h"					//ucos 使用	  
#endif

//=====================================================================================
void delay_ms(u16 time)
{	 	
	u16 i=0; 	
	
	while(time--)
	{
		i=12000;

		while(i--);
	}			
}

//=====================================================================================
//延时nus
//nus为要延时的us数.		    								   
void delay_us(u32 nus)
{		
	u32 temp;	    	 
	SysTick->LOAD=nus*fac_us; 					//时间加载	  		 
	SysTick->VAL=0x00;        					//清空计数器
	SysTick->CTRL|=SysTick_CTRL_ENABLE_Msk ;    //开始倒数	 
	do
	{
		temp=SysTick->CTRL;
	}
	while(temp&0x01&&!(temp&(1<<16)));			//等待时间到达   
	SysTick->CTRL&=~SysTick_CTRL_ENABLE_Msk;    //关闭计数器
	SysTick->VAL =0X00;       					//清空计数器	 
} 





























