#include "delay.h"
#include "sys.h"
////////////////////////////////////////////////////////////////////////////////// 	 
//���ʹ��ucos,����������ͷ�ļ�����.
#if SYSTEM_SUPPORT_UCOS
#include "includes.h"					//ucos ʹ��	  
#endif


void delay_ms(u16 time)
{	 	
u16 i=0; 	
	
		while(time--)
		{
			i=12000;
			
			while(i--);
     }			
    
} 




























