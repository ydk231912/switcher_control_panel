#include "sys.h"
#include "usart.h"	  
#include "led.h"
#include "dma.h"

//=========================================================================================
u16 USART1_RxLen=0;   		//串口接收到的数据数量
u16 USART2_RxLen=0;
u8 USART1_RxOV,USART1_TxOV; //串口1接收完成标志
u8 USART2_RxOV,USART2_TxOV; //串口2接收完成标志
u8 USART3_RxOV,USART3_TxOV; //串口2接收完成标志
u16 Uart1_tempreset,Uart2_tempreset,Uart3_tempreset;

//=====================================================================================
void uart1_init(u32 pclk2,u32 bound)
{
	float temp;
	u16 mantissa;
	u16 fraction;	 

	NVIC_InitTypeDef NVIC_InitStructure;

	temp=(float)(pclk2*1000000)/(bound*16);	//得到USARTDIV
	mantissa=temp;				 			//得到整数部分
	fraction=(temp-mantissa)*16; 			//得到小数部分	 
	mantissa<<=4;
	mantissa+=fraction; 

	RCC->APB2ENR|=1<<2;   					//使能PORTA口时钟  
	RCC->APB2ENR|=1<<14;  					//使能串口时钟 
	GPIOA->CRH&=0XFFFFF00F; 
	GPIOA->CRH|=0X000008B0;					//IO状态设置
		  
	RCC->APB2RSTR|=1<<14;   				//复位串口1
	RCC->APB2RSTR&=~(1<<14);				//停止复位	   	   
 	USART1->BRR=mantissa; 					//波特率设置		 
	USART1->CR1|=0X200C;  					//1位停止,无校验位.

	USART1->CR1|=1<<8;    					//PE中断使能：当USART_SR中的PE为’1’时，产生USART中断。
//	USART1->CR1|=1<<5;    					//接收缓冲区非空中断使能 当USART_SR中的ORE或者RXNE为’1’时，产生USART中断。
	USART1->CR1|=1<<4;	  					//当USART_SR中的IDLE为’1’时，产生USART中断

//	USART1->CR1|=1<<6;    					//当USART_SR中的TC为’1’时，产生USART中断
//	USART1->CR1|=1<<7;	   					//当USART_SR中的TXE为’1’时，产生USART中断    	

	USART1->SR&=~0x0010;
	
//	Usart1 NVIC 配置
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3 ;//抢占优先级3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		//子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);							//根据指定的参数初始化VIC寄存器
}

//=====================================================================================
void USART1_IRQHandler(void)                				//串口1中断服务程序
{
	if(USART_GetITStatus(USART1, USART_IT_IDLE) != RESET)  	//接收中断
	{
		if(((USART1->SR&0x0040)==0x0040))
		{	 	
			USART1->CR1&=~0x0040;	    						 	  		
		}		 
		if((USART1->SR&0x0020)==0x0020)  USART1->SR&=~0x0020;//清除接收中断标志位	
	 
		if((USART1->SR&0x0010)==0x0010)  //当检测到总线空闲时，该位被硬件置位。如果USART_CR1中的IDLEIE为’1’，则产生中断
		{
			USART1->SR&=~0x0010; 
			Uart1_tempreset=USART1->SR;
			Uart1_tempreset=USART1->DR;
		  
			DMA1_Channel5->CCR&=~(1<<0);   				//关闭DMA传输
			USART1->CR3&=~(1<<6);			     		//关闭USART1DMA传输	  	  
// 			Uart1CountRx=DMARX1-DMA1_Channel5->CNDTR;   //检测剩余传输量  						               		  			
			DMA1_Channel5->CNDTR=DMARX1C;	   			//传输数量   注意要与数组对应
			DMA1_Channel5->CCR|=(1<<0);		   			//开启DMA传输  
			USART1->CR3|=1<<6;				       		//开启串口DMA传输
			DMA1->IFCR|=1<<17;				       		//清除DMA中断
			DMA1->IFCR|=1<<13;  	
		}		
		USART1->SR&=~0x0010;
		USART1->CR1&=~0x0040;		
		USART1_RxOV++;
	}   		 
} 

//=====================================================================================
void uart2_init(u32 pclk2,u32 bound){
	
	float temp;
	u16 mantissa;
	u16 fraction;	 
  
	NVIC_InitTypeDef NVIC_InitStructure;

	temp=(float)(pclk2*1000000)/(bound*16);	//得到USARTDIV  da_taa=;
	mantissa=temp;				 			//得到整数部分
	fraction=(temp-mantissa)*16; 			//得到小数部分	 
	mantissa<<=4;
	mantissa+=fraction; 

	RCC->APB2ENR|=1<<2;   					//使能PORTA口时钟 	 
	RCC->APB1ENR|=1<<17;  					//使能串口时钟 

	GPIOA->CRL&=0XFFFF00FF; 
	GPIOA->CRL|=0X00008B00;  				//IO状态设置
		  
	RCC->APB1RSTR|=1<<17;   				//复位串口1
	RCC->APB1RSTR&=~(1<<17);				//停止复位	   	   

 	USART2->BRR=mantissa; 					//波特率设置		 
	USART2->CR1|=0X200C;  					//1位停止,无校验位.

	USART2->CR1|=1<<8;    					//PE中断使能:当USART_SR中的PE为’1’时，产生USART中断。
//	USART1->CR1|=1<<5;    					//接收缓冲区非空中断使能 当USART_SR中的ORE或者RXNE为’1’时，产生USART中断。
	USART2->CR1|=1<<4;	  					//当USART_SR中的IDLE为’1’时，产生USART中断

//	USART1->CR1|=1<<6;    					//当USART_SR中的TC为’1’时，产生USART中断
//	USART1->CR1|=1<<7;	   					//当USART_SR中的TXE为’1’时，产生USART中断
    	
	Uart2_tempreset=USART2->SR;		 		//上电复位中断
	Uart2_tempreset=USART2->DR;
	USART2->SR&=~0x0010;	   
	
//Usart1 NVIC 配置
	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3 ;	//抢占优先级3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;			//子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;				//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);								//根据指定的参数初始化VIC寄存器
}

//===================================================================================
void USART2_IRQHandler(void)                					//串口1中断服务程序
{
	if(((USART2->SR&0x0040)==0x0040))
	{	 	
		USART2->CR1&=~0x0040;
	}

	if((USART2->SR&0x0010)==0x0010)      //当检测到总线空闲时，该位被硬件置位。如果USART_CR1中的IDLEIE为’1’，则产生中断
	{
		USART2->SR&=~0x0010;
														//引脚容易干扰，
		Uart2_tempreset=USART2->SR;
		Uart2_tempreset=USART2->DR;

		DMA1_Channel6->CCR&=~(1<<0);       				//关闭DMA传输
		USART2->CR3&=~(1<<6);			     			//关闭USART1DMA传输	
		  
//		ModUartCountRx2=DMARX2-DMA1_Channel6->CNDTR;	//检测剩余传输量
//		UartRxTimerStartFlag2=1;						//传输完成标志	
					
		DMA1_Channel6->CNDTR=DMARX2;					//传输数量
		DMA1_Channel6->CCR|=(1<<0);						//开启DMA传输  
		USART2->CR3|=1<<6;				    			//开启串口DMA传输
		DMA1->IFCR|=1<<21;								//清除DMA中断
		DMA1->IFCR|=1<<25;			 	 	  	  
		}	

		USART2->SR&=~0x0010;
		USART2->CR1&=~0x0040;	

		USART2_RxOV++;
}

//=========================================================================================
void uart3_init(u32 pclk2,u32 bound)
{
	float temp;
	u16 mantissa;
	u16 fraction;	

	NVIC_InitTypeDef NVIC_InitStructure;

//USART 初始化设置
	temp=(float)(pclk2*1000000)/(bound*16);	//得到USARTDIV  da_taa=;
	mantissa=temp;				 			//得到整数部分
	fraction=(temp-mantissa)*16; 			//得到小数部分	 
	mantissa<<=4;
	mantissa+=fraction; 

	RCC->APB2ENR|=1<<3;   					//使能PORTA口时钟 	 
	RCC->APB1ENR|=1<<18;  					//使能串口时钟 

	GPIOB->CRH&=0XFFFF00FF; 
	GPIOB->CRH|=0X00008B00;  				//IO状态设置
		  
	RCC->APB1RSTR|=1<<18;   				//复位串口1
	RCC->APB1RSTR&=~(1<<18);				//停止复位	   	   

 	USART3->BRR=mantissa; 					//波特率设置		 
	USART3->CR1|=0X200C;  					//1位停止,无校验位.

	USART3->CR1|=1<<8;    					//PE中断使能：当USART_SR中的PE为’1’时，产生USART中断。
//	USART1->CR1|=1<<5;    					//接收缓冲区非空中断使能 当USART_SR中的ORE或者RXNE为’1’时，产生USART中断。
	USART3->CR1|=1<<4;	  					//当USART_SR中的IDLE为’1’时，产生USART中断

//	USART1->CR1|=1<<6;    					//当USART_SR中的TC为’1’时，产生USART中断
//	USART1->CR1|=1<<7;	   					//当USART_SR中的TXE为’1’时，产生USART中断
    	
	Uart3_tempreset=USART3->SR;		 		//上电复位中断
	Uart3_tempreset=USART3->DR;
	USART3->SR&=~0x0010;	   
	
	NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3 ;//抢占优先级3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		//子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);							//根据指定的参数初始化VIC寄存器
}

//===================================================================================
void USART3_IRQHandler(void)                				//串口1中断服务程序
{
	if(((USART3->SR&0x0040)==0x0040))
	{	 	
		USART3->CR1&=~0x0040;
	}

	if((USART3->SR&0x0010)==0x0010)      //当检测到总线空闲时，该位被硬件置位。如果USART_CR1中的IDLEIE为’1’，则产生中断
	{
		USART3->SR&=~0x0010;
									  
		Uart3_tempreset=USART3->SR;
		Uart3_tempreset=USART3->DR;

		DMA1_Channel3->CCR&=~(1<<0);       				//关闭DMA传输
		USART3->CR3&=~(1<<6);			     			//关闭USART1DMA传输	
		  
//		ModUartCountRx2=DMARX2-DMA1_Channel6->CNDTR;   	//检测剩余传输量
//		UartRxTimerStartFlag2=1;						//传输完成标志	
					
		DMA1_Channel3->CNDTR=DMARX3;					//传输数量
		DMA1_Channel3->CCR|=(1<<0);						//开启DMA传输  
		USART3->CR3|=1<<6;				    			//开启串口DMA传输
		DMA1->IFCR|=1<<5;								//清除DMA中断
		DMA1->IFCR|=1<<9;			 	 	  	  
	}	

	USART3->SR&=~0x0010;
	USART3->CR1&=~0x0040;	

	USART3_RxOV++;
  } 










	
	
	
	
	
	
	






