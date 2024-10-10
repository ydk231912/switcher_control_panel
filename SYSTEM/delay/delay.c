#include "delay.h"
#include "sys.h"
////////////////////////////////////////////////////////////////////////////////// 	 
//如果使用ucos,则包括下面的头文件即可.
#if SYSTEM_SUPPORT_UCOS
#include "includes.h"					//ucos 使用	  
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




























