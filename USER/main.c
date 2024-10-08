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
u8 dat=0;

int main(void)
{	 
//	delay_init();	    	 	//延时函数初始化	 

//	JTAG_Set(JTAG_SWD_DISABLE);   
//  JTAG_Set(SWD_ENABLE);    
	NVIC_Configuration(); 	 	//设置NVIC中断分组2:2位抢占优先级，2位响应优先级
	 
	uart1_init(72,115200);	 	//串口初始化为9600
	uart2_init(36,115200);
	uart3_init(36,115200);  
	
 	LED_Init();			     	//LED端口初始化
	
	TIM3_Int_Init(20,7199);     //定时器2
	TIM2_Int_Init(100,7199);
	
//	SPI1_Init();
	SPI2_Init();
//	SPI3_Init();
//  FLASH_IO_Init();
	     
	DMA_USART1_Tx_Config(DMA1_Channel4,(u32)&(USART1->DR),(u32)&ucByteBufTX1,DMATX1); 
	DMA_USART1_Rx_Config(DMA1_Channel5,(u32)&(USART1->DR),(u32)&ucByteBufRX1,DMARX1);

	DMA_USART2_Tx_Config(DMA1_Channel7,(u32)&(USART2->DR),(u32)&ucByteBufTX2,DMATX2C);
	DMA_USART2_Rx_Config(DMA1_Channel6,(u32)&(USART2->DR),(u32)&ucByteBufRX2,DMARX2C);

	DMA_USART3_Tx_Config(DMA1_Channel2,(u32)&(USART3->DR),(u32)&ucByteBufTX3,DMATX3);
	DMA_USART3_Rx_Config(DMA1_Channel3,(u32)&(USART3->DR),(u32)&ucByteBufRX3,DMARX3); 
	 
//	DMA_SPI2_Tx_Config();

	CAN1_Mode_Init(1,3,2,6,0); 

	OLED_Init();

	CSse=0;
	initialize();
	OLED_Fill(0,0,256,128,0x00);
	CSse=1;
	initialize();
	OLED_Fill(0,0,256,128,0x00);
	CSse=2;
	initialize();
	OLED_Fill(0,0,256,128,0x00);

	KEY_Init();
	SPI2_WriteByte(&dat,2);

	address_selection();
		 
	delay_ms(100);
		 
	while(1)
	{	    
		fun();	
// 		OLED_Fill(0,0,256,128,0x00);
		if(oledon==37)
		{		
			oledon=0;
			CSse=0;
			OLED_ShowString(12,10,oled1,16,0);
			OLED_ShowString(12,32,oled2,16,0);
			CSse=1;
			OLED_ShowString(12,10,oled3,16,0);
			OLED_ShowString(12,32,oled4,16,0);
			CSse=2;
			OLED_ShowString(12,10,oled5,16,0);
			OLED_ShowString(12,32,oled6,16,0);
		}
	}			    
}
		   
	
		
		
		
		
	
	
	

