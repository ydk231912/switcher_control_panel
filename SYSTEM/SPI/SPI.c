#include "spi.h"
#include "delay.h"
#include "fun.h"
#include "led.h"
u8 SPI_RW(u8 buchar);	

u8 SPIbuf1[10];
u8 SPIbuf2[10];
u8 SPI1_ReadWriteByte(u8 TxData);
//从机模式
void SPI1_Init(void)
{
	GPIO_InitTypeDef 	GPIO_InitStructure;
	SPI_InitTypeDef   	SPI_InitStructure;	   
  NVIC_InitTypeDef NVIC_InitStructure;
	
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_SPI1 | RCC_APB2Periph_AFIO, ENABLE);	

	/* 初始化SCK、MISO、MOSI引脚 */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_SetBits(GPIOA,GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7);

	/* 初始化CS引脚 */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_Out_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_SetBits(GPIOA, GPIO_Pin_11);

	/* 初始化配置STM32 SPI1 */
	
	SPI_InitStructure.SPI_BaudRatePrescaler=SPI_BaudRatePrescaler_8;
	SPI_InitStructure.SPI_CPHA=SPI_CPHA_2Edge;							//数据捕获于第1个时钟沿
	SPI_InitStructure.SPI_CPOL=SPI_CPOL_High;							//时钟悬空低
	SPI_InitStructure.SPI_CRCPolynomial=7;								 //CRC多项式为7
	SPI_InitStructure.SPI_DataSize=SPI_DataSize_8b;						//SPI发送接收8位帧结构
	SPI_InitStructure.SPI_Direction=SPI_Direction_2Lines_FullDuplex;	//SPI设置为双线双向全双工
	SPI_InitStructure.SPI_Mode=SPI_Mode_Slave ;						//设置为从SPI
	SPI_InitStructure.SPI_FirstBit=SPI_FirstBit_MSB;			 //数据传输从MSB位开始
	SPI_InitStructure.SPI_NSS=SPI_NSS_Soft ;								//NSS由外部管脚管理
	
	SPI_I2S_ITConfig(SPI1,SPI_I2S_IT_RXNE,ENABLE);   //开启接收中断
	SPI_Init(SPI1,&SPI_InitStructure);							 //根据SPI_InitStruct中指定的参数初始化外设SPI1寄存器
	
	NVIC_InitStructure.NVIC_IRQChannel = SPI1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3 ;//抢占优先级3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		//子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器

	SPI_Cmd(SPI1,ENABLE);	//STM32使能SPI1
 // SPI1_ReadWriteByte(0xff);//启动传输		 
}
//============================================================================
void SPI1_IRQHandler(void) 
{
	/*
	if(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_IT_RXNE))
	{
		Spi1RxBuf[SPIRXcnt]=SPI_I2S_ReceiveData(SPI1);
		SPIRXcnt++;
		if(SPIRXcnt>=SPI1FH1len){SPIRXcnt=0;SPIon=1;}
		while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET){}
		SPI_I2S_SendData(SPI1, Spi1TxBuf[SPITXcnt]);
		SPITXcnt++;
		if(SPITXcnt>=SPI1FH1len)SPITXcnt=0;
//		LED2=!LED2;
  }
	*/
}



u8 SPI1_ReadWriteByte(u8 TxData)
{		
	u8 retry=0;	
	
	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET) //检查指定的SPI标志位设置与否:发送缓存空标志位
		{
		 retry++;
		 if(retry>200)return 0;
		}			  
	   SPI_I2S_SendData(SPI1, TxData); //通过外设SPIx发送一个数据
	   retry=0;

	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET) //检查指定的SPI标志位设置与否:接受缓存非空标志位
		{
		 retry++;
		 if(retry>200)return 0;
		}	  						    
	return SPI_I2S_ReceiveData(SPI1); //返回通过SPIx最近接收的数据					    
}

//========================================================================================
void SPI1_ReadorWriteByte(u8 *TxData,u8 *RxData,u16 len)
{
	u16 i;
	
	for(i=0;i<len;i++)
	{
		RxData[i]=SPI1_ReadWriteByte(TxData[i]);	
  }
}


//========================================================================================
//==================================================================================================

void SPI2_Init(void)
{
 	GPIO_InitTypeDef GPIO_InitStructure;
  SPI_InitTypeDef  SPI_InitStructure;

	RCC_APB2PeriphClockCmd(	RCC_APB2Periph_GPIOB, ENABLE );//PORTB时钟使能 
	RCC_APB1PeriphClockCmd(	RCC_APB1Periph_SPI2,  ENABLE );//SPI2时钟使能 	
 
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;  //PB13/14/15复用推挽输出 
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);//初始化GPIOB

 	GPIO_SetBits(GPIOB,GPIO_Pin_13|GPIO_Pin_14|GPIO_Pin_15);  //PB13/14/15上拉

	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;  //设置SPI单向或者双向的数据模式:SPI设置为双线双向全双工
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;		//设置SPI工作模式:设置为主SPI
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;		//设置SPI的数据大小:SPI发送接收8位帧结构
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;		//串行同步时钟的空闲状态为高电平
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;	//串行同步时钟的第二个跳变沿（上升或下降）数据被采样
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;		//NSS信号由硬件（NSS管脚）还是软件（使用SSI位）管理:内部NSS信号有SSI位控制
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;		//定义波特率预分频的值:波特率预分频值为256
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;	//指定数据传输从MSB位还是LSB位开始:数据传输从MSB位开始
	SPI_InitStructure.SPI_CRCPolynomial = 7;	//CRC值计算的多项式
	SPI_Init(SPI2, &SPI_InitStructure);  //根据SPI_InitStruct中指定的参数初始化外设SPIx寄存器
 
	SPI_Cmd(SPI2, ENABLE); //使能SPI外设
	
	SPI2_ReadWriteByte(0xff);//启动传输		 SN74VC1T
 

}   

void SPI2_SetSpeed(u8 SPI_BaudRatePrescaler)
{
  assert_param(IS_SPI_BAUDRATE_PRESCALER(SPI_BaudRatePrescaler));
	SPI2->CR1&=0XFFC7;
	SPI2->CR1|=SPI_BaudRatePrescaler;	//设置SPI2速度 
	SPI_Cmd(SPI2,ENABLE); 

} 

//SPIx 读写一个字节
//TxData:要写入的字节
//返回值:读取到的字节
u8 SPI2_ReadWriteByte(u8 TxData)
{		
	u8 retry=0;				 	
	while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE) == RESET) //检查指定的SPI标志位设置与否:发送缓存空标志位
		{
		retry++;
		if(retry>200)return 0;
		}			  
	SPI_I2S_SendData(SPI2, TxData); //通过外设SPIx发送一个数据
	retry=0;

//	while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_RXNE) == RESET) //检查指定的SPI标志位设置与否:接受缓存非空标志位
//		{
	//	retry++;
	//	if(retry>200)return 0;
//		}	  						    
	return SPI_I2S_ReceiveData(SPI2); //返回通过SPIx最近接收的数据					    
}

//=====================================================================================

//连续多字节发送
u8 SPI2_WriteByte(u8 * buff,u16 len)
{
	u8 retry=0;
	u8 TxData;
	u16 i;
	
	for(i=0;i<len;i++)
	{
		TxData=*(buff+i);
	 while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE) == RESET) //检查指定的SPI标志位设置与否:发送缓存空标志位
		{
		 retry++;
		 if(retry>200)return 0;
		}			  
	 SPI_I2S_SendData(SPI2, TxData); //通过外设SPIx发送一个数据
	 retry=0;
	}
	return 0;
}

//=============================================================================================
void SPI3_Init(void)
{
  GPIO_InitTypeDef    GPIO_InitStructure;
	SPI_InitTypeDef   	SPI_InitStructure;	
	
	RCC_APB2PeriphClockCmd(	RCC_APB2Periph_GPIOA|RCC_APB2Periph_AFIO|RCC_APB2Periph_GPIOB, ENABLE );
	RCC_APB1PeriphClockCmd(	RCC_APB1Periph_SPI3,  ENABLE );//SPI2时钟使能 	
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);//改变管脚映射，关闭JTAG，使能SWD
	RCC_PCLK1Config(RCC_HCLK_Div2);                        //36M时钟
	
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
//	GPIO_SetBits(GPIOB,GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7);

	/* 初始化CS引脚 */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_Out_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
//	GPIO_SetBits(GPIOA, GPIO_Pin_15);
	
	
	/* 初始化配置STM32 SPI1 */
	SPI_InitStructure.SPI_Direction=SPI_Direction_2Lines_FullDuplex;	//SPI设置为双线双向全双工
	SPI_InitStructure.SPI_Mode=SPI_Mode_Slave;							//设置为主SPI
	SPI_InitStructure.SPI_DataSize=SPI_DataSize_8b;						//SPI发送接收8位帧结构
	SPI_InitStructure.SPI_CPOL=SPI_CPOL_Low;							//时钟悬空低
	SPI_InitStructure.SPI_CPHA=SPI_CPHA_2Edge;							//数据捕获于第1个时钟沿
	SPI_InitStructure.SPI_NSS=SPI_NSS_Soft;								//NSS由外部管脚管理
//SPI_InitStructure.SPI_BaudRatePrescaler=SPI_BaudRatePrescaler_2;	//波特率预分频值为2
  SPI_InitStructure.SPI_BaudRatePrescaler=SPI_BaudRatePrescaler_64;
	SPI_InitStructure.SPI_FirstBit=SPI_FirstBit_MSB;					//数据传输从MSB位开始
	SPI_InitStructure.SPI_CRCPolynomial=7;								//CRC多项式为7
	SPI_Init(SPI3,&SPI_InitStructure);									//根据SPI_InitStruct中指定的参数初始化外设SPI1寄存器
	
	
	SPI_Cmd(SPI3, ENABLE); //使能SPI外设
	
	
}

//SPIx 读写一个字节
//TxData:要写入的字节
//返回值:读取到的字节
u8 SPI3_ReadWriteByte(u8 TxData)
{		
	u8 retry=0;				 	
	while (SPI_I2S_GetFlagStatus(SPI3, SPI_I2S_FLAG_TXE) == RESET) //检查指定的SPI标志位设置与否:发送缓存空标志位
		{
		retry++;
		if(retry>200)return 0;
		}			  
	SPI_I2S_SendData(SPI3, TxData); //通过外设SPIx发送一个数据
	retry=0;

//	while (SPI_I2S_GetFlagStatus(SPI3, SPI_I2S_FLAG_RXNE) == RESET) //检查指定的SPI标志位设置与否:接受缓存非空标志位
	//	{
	//	retry++;
	//	if(retry>200)return 0;
	//	}	  						    
	return SPI_I2S_ReceiveData(SPI3); //返回通过SPIx最近接收的数据					    
}


//====================================================================================================

u8 SPI3_WriteByte(u8 * buff,u16 len)
{
	u8 retry=0;
	u8 TxData;
	u16 i;
	
	for(i=0;i<len;i++)
	{
		TxData=*(buff+i);
	 while (SPI_I2S_GetFlagStatus(SPI3, SPI_I2S_FLAG_TXE) == RESET) //检查指定的SPI标志位设置与否:发送缓存空标志位
		{
		 retry++;
		 if(retry>200)return 0;
		}			  
	 SPI_I2S_SendData(SPI3, TxData); //通过外设SPIx发送一个数据
	 retry=0;
	}
	return 0;
}






























//=====模拟SPI读写======================================================================


















