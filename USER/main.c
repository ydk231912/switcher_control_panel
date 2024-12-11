#include "led.h"
#include "delay.h"
#include "sys.h"
#include "usart.h"	 
#include "dma.h"
#include "tim.h"
#include "fun.h"
#include "spi.h"
#include "oled.h"
#include "can.h"
#include "adc.h"

u8 dat=0;
s8 OLEDW=0;

 int main(void)
 {	 
	delay_init();	    	 //延时函数初始化	 

//	JTAG_Set(JTAG_SWD_DISABLE); /*??JTAG??*/   
//  JTAG_Set(SWD_ENABLE);    /*??SWD?? ???????SWD????*/ 
	NVIC_Configuration(); 	 //设置NVIC中断分组2:2位抢占优先级，2位响应优先级
	 
	uart1_init(72,115200);	 	//串口初始化为9600
	uart2_init(36,115200);
	uart3_init(36,500000);  
 	LED_Init();			     //LED端口初始化
	TIM3_Int_Init(20,7199);     //定时器2
  TIM2_Int_Init(100,7199);
	
	//SPI1_Init();
	SPI2_Init();
	
// FLASH_IO_Init();
	     
  DMA_USART1_Tx_Config(DMA1_Channel4,(u32)&(USART1->DR),(u32)&ucByteBufTX1,DMATX1); 
	DMA_USART1_Rx_Config(DMA1_Channel5,(u32)&(USART1->DR),(u32)&ucByteBufRX1,DMARX1);
	 
	DMA_USART2_Tx_Config(DMA1_Channel7,(u32)&(USART2->DR),(u32)&ucByteBufTX2,DMATX2);
	DMA_USART2_Rx_Config(DMA1_Channel6,(u32)&(USART2->DR),(u32)&ucByteBufRX2,DMARX2);
	 
	DMA_USART3_Tx_Config(DMA1_Channel2,(u32)&(USART3->DR),(u32)&ucByteBufTX3,DMATX3);
	DMA_USART3_Rx_Config(DMA1_Channel3,(u32)&(USART3->DR),(u32)&ucByteBufRX3,DMARX3); 
	
	CAN1_Mode_Init(1,3,2,6,0);  

	KEY_Init();
	Adc_Init();	 
	SPI2_WriteByte(&dat,2);
	
	SPI3_Init();
	OLED_Init();			//初始化OLED 	
  OLED_Clear(); 
		
  IWDG_Init(16,3700);
		

	while(1)
	{	    
	//99=6   0= -16   100=12
    fun();
			
	//	OLED_Clear();
	//	OLED_ShowChar(48,6,2);//显示ASCII字符	   		
 //  OLED_ShowString(20,4,"2014/05/01"); 
	//	if((oleddata>=0)&&(oleddata<=10))OLEDW=-16;
	//	if((oleddata>=10)&&(oleddata<100))OLEDW=6;
	//	if(oleddata>=100)OLEDW=12;
  //    OLED_ShowNum(0,0,oleddata,3,64);
//		OLED_ShowString(20,4,"67"); 
     IWDG_Feed();
		
}			    

		}
		   
	
	
	
	

