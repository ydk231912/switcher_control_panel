#include "dma.h"
#include "led.h"
#include "usart.h"
#include "rgbw.h"
#include "fun.h"

volatile u8  ucByteBufRX1[DMARX1];    //串口1通讯数组
volatile u8  ucByteBufTX1[DMATX1];

volatile u8  ucByteBufRX2[DMARX2];    //串口1通讯数组
volatile u8  ucByteBufTX2[DMATX2];

volatile u8  ucByteBufRX3[DMARX3];    //串口3通讯数组
volatile u8  ucByteBufTX3[DMATX3];
u16 DMA1_MEM_LEN;//保存DMA每次数据传送的长度 
u16 DMA1_MEM_LEN2;//保存DMA每次数据传送的长度 
//DMA1的各通道配置
//这里的传输形式是固定的,这点要根据不同的情况来修改
//从存储器->外设模式/8位数据宽度/存储器增量模式
//DMA_CHx:DMA通道CHx
//cpar:外设地址
//cmar:存储器地址
//cndtr:数据传输量 

u16 temp2;

void DMA_USART1_Tx_Config(DMA_Channel_TypeDef* DMA_CHx,u32 cpar,u32 cmar,u16 cndtr)
{

	NVIC_InitTypeDef NVIC_InitStructure;
 	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);	//使能DMA传输
	
  RCC->AHBENR|=1<<0;			//开启DMA1时钟
//	delay_ms(500);			    //等待DMA时钟稳定
	DMA_CHx->CPAR=cpar; 	 	//DMA1 外设地址 
	DMA_CHx->CMAR=(u32)cmar; 	//DMA1,存储器地址
	DMA1_MEM_LEN=cndtr;      	//保存DMA传输数据量
	DMA_CHx->CNDTR=cndtr;    	//DMA1,传输数据量
	DMA_CHx->CCR=0X00000000;	//复位
	DMA_CHx->CCR|=1<<4;  		//从存储器读  
	DMA_CHx->CCR|=0<<5;  		//普通模式
	DMA_CHx->CCR|=0<<6; 		//外设地址非增量模式
	DMA_CHx->CCR|=1<<7; 	 	//存储器增量模式
	DMA_CHx->CCR|=0<<8; 	 	//外设数据宽度为8位
	DMA_CHx->CCR|=0<<10; 		//存储器数据宽度8位
	DMA_CHx->CCR|=0<<12; 		//中等优先级
	DMA_CHx->CCR|=0<<14; 		//非存储器到存储器模式
	DMA_CHx->CCR|=1<<1;			//允许传输完成中断
	
	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel4_IRQn;  //设置DMA中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3 ;//抢占优先级3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		//子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器
}  
//==============================================================

void DMA_USART1_Rx_Config(DMA_Channel_TypeDef* DMA_CHx,u32 cpar,u32 cmar,u16 cndtr)
{
	
	NVIC_InitTypeDef NVIC_InitStructure;
 	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);	//使能DMA传输
	
  RCC->AHBENR|=1<<0;			//开启DMA1时钟
//	delay_ms(500);			    //等待DMA时钟稳定
	DMA_CHx->CPAR=cpar; 	 	//DMA1 外设地址 
	DMA_CHx->CMAR=(u32)cmar; 	//DMA1,存储器地址
	DMA1_MEM_LEN=cndtr;      	//保存DMA传输数据量
	DMA_CHx->CNDTR=cndtr;    	//DMA1,传输数据量
	DMA_CHx->CCR=0X00000000;	//复位
	DMA_CHx->CCR|=1<<1;			//允许传输完成中断
	DMA_CHx->CCR|=0<<4;  		//从外设读
	DMA_CHx->CCR|=0<<5;  		//普通模式
	DMA_CHx->CCR|=0<<6; 		//外设地址非增量模式
	DMA_CHx->CCR|=1<<7; 	 	//存储器增量模式
	DMA_CHx->CCR|=0<<8; 	 	//外设数据宽度为8位
	DMA_CHx->CCR|=0<<10; 		//存储器数据宽度8位
	DMA_CHx->CCR|=1<<12; 		//中等优先级
	DMA_CHx->CCR|=0<<14; 		//非存储器到存储器模式
 	USART1->CR3|=1<<6;			//开启USART1DMA传输
	DMA_CHx->CCR|=1<<0;			//默认开启传输
	DMA_CHx->CCR|=1<<1;			//允许传输完成中断
 
	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel5_IRQn;  //设置DMA中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3 ;//抢占优先级3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		//子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器	
}

//==========================================================
//开启一次DMA传输
void DMA_USART1_Tx(DMA_Channel_TypeDef*DMA_CHx,u16 len)
{ 
	USART1->CR3=1<<7;			   //开启串口1DMA
	DMA1->IFCR|=1<<13;          //清除通道4传输完成标志 
	DMA_CHx->CCR&=~(1<<0);       //关闭DMA传输 
	DMA_CHx->CNDTR=len; //DMA1,传输数据量 		  
	DMA_CHx->CCR|=1<<0;          //开启DMA传输
	USART1->CR3|=1<<6;			   //提前打开串口通讯  防止传输延迟
	
}	  
//===================================================================
 
void DMA1_Channel4_IRQHandler (void)
{
	DMA1->IFCR|=1<<13;     //清除通道4传输完成标志
	DMA1->IFCR|=1<<12;     ////清除通道4全局传输完成标志
  USART1->CR3&=~(1<<7);	 //关闭串口发送DMA 
	USART1_TxOV++;
}
//===================================================================
void DMA1_Channel5_IRQHandler(void)	//清除读中断//清除DMA中断
{
   DMA1->IFCR|=1<<17;     //清除通道4传输完成标志 
	
	 if(DMA_GetITStatus(DMA1_IT_TC5))
		 {
       DMA_Cmd(DMA1_Channel5, DISABLE);
       DMA_ClearITPendingBit(DMA1_IT_TC5); //关中断	 
     }
}



//============================================================================

//==============================================================================

void DMA_USART2_Tx_Config(DMA_Channel_TypeDef* DMA_CHx,u32 cpar,u32 cmar,u16 cndtr)
{
	NVIC_InitTypeDef NVIC_InitStructure;
 	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);	//使能DMA传输
	
  RCC->AHBENR|=1<<0;			//开启DMA1时钟
//	delay_ms(500);			//等待DMA时钟稳定
	DMA_CHx->CPAR=cpar; 	 	//DMA1 外设地址 
	DMA_CHx->CMAR=(u32)cmar; 	//DMA1,存储器地址
	DMA1_MEM_LEN2=cndtr;      	//保存DMA传输数据量
	DMA_CHx->CNDTR=cndtr;    	//DMA1,传输数据量
	DMA_CHx->CCR=0X00000000;	//复位
	DMA_CHx->CCR|=1<<4;  		//从存储器读  
	DMA_CHx->CCR|=0<<5;  		//普通模式
	DMA_CHx->CCR|=0<<6; 		//外设地址非增量模式
	DMA_CHx->CCR|=1<<7; 	 	//存储器增量模式
	DMA_CHx->CCR|=0<<8; 	 	//外设数据宽度为8位
	DMA_CHx->CCR|=0<<10; 		//存储器数据宽度8位
	DMA_CHx->CCR|=0<<12; 		//中等优先级
	DMA_CHx->CCR|=0<<14; 		//非存储器到存储器模式
	DMA_CHx->CCR|=1<<1;			//允许传输完成中断
	
	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel7_IRQn;  //设置DMA中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=2 ;//抢占优先级3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;		//子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	    //根据指定的参数初始化VIC寄存器
}  
//=====================================================================================

void DMA_USART2_Rx_Config(DMA_Channel_TypeDef* DMA_CHx,u32 cpar,u32 cmar,u16 cndtr)
{
	
	NVIC_InitTypeDef NVIC_InitStructure;
 	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);	//使能DMA传输
	
  RCC->AHBENR|=1<<0;			//开启DMA1时钟
//	delay_ms(500);			    //等待DMA时钟稳定
	DMA_CHx->CPAR=cpar; 	 	//DMA1 外设地址 
	DMA_CHx->CMAR=(u32)cmar; 	//DMA1,存储器地址
	DMA1_MEM_LEN2=cndtr;      	//保存DMA传输数据量
	DMA_CHx->CNDTR=cndtr;    	//DMA1,传输数据量
	DMA_CHx->CCR=0X00000000;	//复位
	DMA_CHx->CCR|=1<<1;			//允许传输完成中断
	DMA_CHx->CCR|=0<<4;  		//从外设读
	DMA_CHx->CCR|=0<<5;  		//普通模式
	DMA_CHx->CCR|=0<<6; 		//外设地址非增量模式
	DMA_CHx->CCR|=1<<7; 	 	//存储器增量模式
	DMA_CHx->CCR|=0<<8; 	 	//外设数据宽度为8位
	DMA_CHx->CCR|=0<<10; 		//存储器数据宽度8位
	DMA_CHx->CCR|=1<<12; 		//中等优先级
	DMA_CHx->CCR|=0<<14; 		//非存储器到存储器模式
	USART2->CR3|=1<<6;			//开启USART2DMA传输
	DMA_CHx->CCR|=1<<0;			//默认开启传输
 
	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel6_IRQn;  //设置DMA中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3 ;//抢占优先级3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		//子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器	
}
//===================================================================================
//开启一次DMA传输
void DMA_USART2_Tx(DMA_Channel_TypeDef*DMA_CHx,u16 len)
{ 
	USART2->CR3=1<<7;			   //使能发送时的DMA模式
	DMA1->IFCR|=1<<25;             //清除通道4传输完成标志 
	DMA_CHx->CCR&=~(1<<0);         //关闭DMA传输 
	DMA_CHx->CNDTR=len; //DMA1,传输数据量 		  
	DMA_CHx->CCR|=1<<0;            //开启DMA传输
	USART2->CR3|=1<<6;			   //提前打开串口通讯  防止传输延迟 

}	  

 
void DMA1_Channel7_IRQHandler (void)
{
	DMA1->IFCR|=1<<25;     //清除通道4传输完成标志
	DMA1->IFCR|=1<<24;
  USART2->CR3&=~(1<<7);	 //关闭串口发送DMA   
  
}


//======================================================================================


//DMA接收中断是在固定数据，一般不适应，关闭DMA接收中断
void DMA1_Channel6_IRQHandler (void)
{
 if(DMA_GetITStatus(DMA1_IT_TC6) != RESET)
 {
  DMA1->IFCR|=1<<17;     //清除通道4传输完成标志
 }
}
//=========================================================================================

//==============================================================================

void DMA_USART3_Tx_Config(DMA_Channel_TypeDef* DMA_CHx,u32 cpar,u32 cmar,u16 cndtr)
{
	
	NVIC_InitTypeDef NVIC_InitStructure;
 	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);	//使能DMA传输
	
  RCC->AHBENR|=1<<0;			//开启DMA1时钟
 //	delay_ms(500);			//等待DMA时钟稳定
	DMA_CHx->CPAR=cpar; 	 	//DMA1 外设地址 
	DMA_CHx->CMAR=(u32)cmar; 	//DMA1,存储器地址
	DMA1_MEM_LEN2=cndtr;      	//保存DMA传输数据量
	DMA_CHx->CNDTR=cndtr;    	//DMA1,传输数据量
	DMA_CHx->CCR=0X00000000;	//复位
	DMA_CHx->CCR|=1<<4;  		//从存储器读  
	DMA_CHx->CCR|=0<<5;  		//普通模式
	DMA_CHx->CCR|=0<<6; 		//外设地址非增量模式
	DMA_CHx->CCR|=1<<7; 	 	//存储器增量模式
	DMA_CHx->CCR|=0<<8; 	 	//外设数据宽度为8位
	DMA_CHx->CCR|=0<<10; 		//存储器数据宽度8位
	DMA_CHx->CCR|=0<<12; 		//中等优先级
	DMA_CHx->CCR|=0<<14; 		//非存储器到存储器模式
	DMA_CHx->CCR|=1<<1;			//允许传输完成中断

	
	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel2_IRQn;  //设置DMA中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3 ;//抢占优先级3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		//子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	    //根据指定的参数初始化VIC寄存器
}  

//===================================================================================
//开启一次DMA传输
void DMA_USART3_Tx(DMA_Channel_TypeDef*DMA_CHx,u16 len)
{ 
	USART3->CR3=1<<7;			   //使能发送时的DMA模式
	DMA1->IFCR|=1<<5;             //清除通道4传输完成标志 
	DMA_CHx->CCR&=~(1<<0);         //关闭DMA传输 
	DMA_CHx->CNDTR=len;        //DMA1,传输数据量 		  
	DMA_CHx->CCR|=1<<0;            //开启DMA传输
	USART3->CR3|=1<<6;			   //提前打开串口通讯  防止传输延迟  

}	  

 
void DMA1_Channel2_IRQHandler (void)
{
	DMA1->IFCR|=1<<5;     //清除通道4传输完成标志
	DMA1->IFCR|=1<<9;
  USART3->CR3&=~(1<<7);	 //关闭串口发送DMA   
  USART3_TxOV++;
//	LED2=!LED2;
}
//=====================================================================================

void DMA_USART3_Rx_Config(DMA_Channel_TypeDef* DMA_CHx,u32 cpar,u32 cmar,u16 cndtr)
{
	
	
	NVIC_InitTypeDef NVIC_InitStructure;
 	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);	//使能DMA传输
	
  RCC->AHBENR|=1<<0;			//开启DMA1时钟
//	delay_ms(500);			    //等待DMA时钟稳定
	DMA_CHx->CPAR=cpar; 	 	//DMA1 外设地址 
	DMA_CHx->CMAR=(u32)cmar; 	//DMA1,存储器地址
	DMA1_MEM_LEN2=cndtr;      	//保存DMA传输数据量
	DMA_CHx->CNDTR=cndtr;    	//DMA1,传输数据量
	DMA_CHx->CCR=0X00000000;	//复位
	DMA_CHx->CCR|=1<<1;			//允许传输完成中断
	DMA_CHx->CCR|=0<<4;  		//从外设读
	DMA_CHx->CCR|=0<<5;  		//普通模式
	DMA_CHx->CCR|=0<<6; 		//外设地址非增量模式
	DMA_CHx->CCR|=1<<7; 	 	//存储器增量模式
	DMA_CHx->CCR|=0<<8; 	 	//外设数据宽度为8位
	DMA_CHx->CCR|=0<<10; 		//存储器数据宽度8位
	DMA_CHx->CCR|=1<<12; 		//中等优先级
	DMA_CHx->CCR|=0<<14; 		//非存储器到存储器模式
	USART3->CR3|=1<<6;			//开启USART2DMA传输
	DMA_CHx->CCR|=1<<0;			//默认开启传输
 
	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel3_IRQn;  //设置DMA中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3 ;//抢占优先级3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		//子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器	
}

//======================================================================================


//DMA接收中断是在固定数据，一般不适应，关闭DMA接收中断
void DMA1_Channel3_IRQHandler (void)
{
 if(DMA_GetITStatus(DMA1_IT_TC3) != RESET)
 {
	DMA_ClearITPendingBit(DMA1_IT_TC3);
  DMA_ClearFlag(DMA1_FLAG_TC3); //清除DMA中断标志
//USART_ClearFlag(USART1,USART_FLAG_TC); //清除串口中断标志
//DMA_Cmd(DMA1_Channel5, DISABLE );      //关闭DMA传输
 }
}//==========================================================================================

void DMA_SPI2_Tx_Config(void)
{
	 DMA_InitTypeDef  DMA_InitStructure;
	 NVIC_InitTypeDef NVIC_InitStructure;
 	 RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);	//使能DMA传输
	
	
   DMA_DeInit(DMA1_Channel5);
   DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)(&SPI2->DR); //DMA外设ADC基地址
   DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)gWs2812bDat_SPI;  //DMA内存基地址
   DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;       //数据传输方向，从内存读取发送到外设
   DMA_InitStructure.DMA_BufferSize = WS2812B_AMOUNT * 32;  //DMA通道的DMA缓存的大小
   DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;   //外设地址寄存器不变
   DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;            //内存地址寄存器递增  
   DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;   //数据宽度为8位
   DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;            //数据宽度为8位
   DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;                       //工作在正常缓存模式
   DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;         //DMA通道 x拥有中优先级 
   DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;                 //DMA通道x没有设置为内存到内存传输   
   DMA_Init(DMA1_Channel5, &DMA_InitStructure);  

	if(DMA_GetITStatus(DMA1_IT_TC5)!= RESET)
		{
     DMA_ClearITPendingBit(DMA1_IT_TC5);
    }
	
		
	DMA_ITConfig(DMA1_Channel5, DMA_IT_TC, ENABLE);
  SPI_I2S_DMACmd(SPI2, SPI_I2S_DMAReq_Tx, ENABLE);
  DMA_Cmd(DMA1_Channel5, DISABLE);
	
	
	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel5_IRQn;  //设置DMA中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3 ;//抢占优先级3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		//子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	    //根据指定的参数初始化VIC寄存器
}  

//===================================================================================
//开启一次DMA传输
void DMA_SPI2_Tx(u8 *buff,u16 LEN)
{ 

	 DMA_Cmd(DMA1_Channel5,DISABLE);
   DMA1_Channel5->CNDTR = LEN;
	 DMA1_Channel5->CMAR=(u32)buff;
   DMA_Cmd(DMA1_Channel5,ENABLE);
   
	
	
}	  


