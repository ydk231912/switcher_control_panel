#include "spi.h"
#include "delay.h"
#include "fun.h"
#include "led.h"
u8 SPI_RW(u8 buchar);	

u8 SPIbuf1[10];
u8 SPIbuf2[10];
u8 SPI1_ReadWriteByte(u8 TxData);
//´Ó»úÄ£Ê½
void SPI1_Init(void)
{
	GPIO_InitTypeDef 	GPIO_InitStructure;
	SPI_InitTypeDef   	SPI_InitStructure;	   
  NVIC_InitTypeDef NVIC_InitStructure;
	
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_SPI1 | RCC_APB2Periph_AFIO, ENABLE);	

	/* ³õÊ¼»¯SCK¡¢MISO¡¢MOSIÒý½Å */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_SetBits(GPIOA,GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7);

	/* ³õÊ¼»¯CSÒý½Å */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_Out_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_SetBits(GPIOA, GPIO_Pin_11);

	/* ³õÊ¼»¯ÅäÖÃSTM32 SPI1 */
	
	SPI_InitStructure.SPI_BaudRatePrescaler=SPI_BaudRatePrescaler_8;
	SPI_InitStructure.SPI_CPHA=SPI_CPHA_2Edge;							//Êý¾Ý²¶»ñÓÚµÚ1¸öÊ±ÖÓÑØ
	SPI_InitStructure.SPI_CPOL=SPI_CPOL_High;							//Ê±ÖÓÐü¿ÕµÍ
	SPI_InitStructure.SPI_CRCPolynomial=7;								 //CRC¶àÏîÊ½Îª7
	SPI_InitStructure.SPI_DataSize=SPI_DataSize_8b;						//SPI·¢ËÍ½ÓÊÕ8Î»Ö¡½á¹¹
	SPI_InitStructure.SPI_Direction=SPI_Direction_2Lines_FullDuplex;	//SPIÉèÖÃÎªË«ÏßË«ÏòÈ«Ë«¹¤
	SPI_InitStructure.SPI_Mode=SPI_Mode_Slave ;						//ÉèÖÃÎª´ÓSPI
	SPI_InitStructure.SPI_FirstBit=SPI_FirstBit_MSB;			 //Êý¾Ý´«Êä´ÓMSBÎ»¿ªÊ¼
	SPI_InitStructure.SPI_NSS=SPI_NSS_Soft ;								//NSSÓÉÍâ²¿¹Ü½Å¹ÜÀí
	
	SPI_I2S_ITConfig(SPI1,SPI_I2S_IT_RXNE,ENABLE);   //¿ªÆô½ÓÊÕÖÐ¶Ï
	SPI_Init(SPI1,&SPI_InitStructure);							 //¸ù¾ÝSPI_InitStructÖÐÖ¸¶¨µÄ²ÎÊý³õÊ¼»¯ÍâÉèSPI1¼Ä´æÆ÷
	
	NVIC_InitStructure.NVIC_IRQChannel = SPI1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3 ;//ÇÀÕ¼ÓÅÏÈ¼¶3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		//×ÓÓÅÏÈ¼¶3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQÍ¨µÀÊ¹ÄÜ
	NVIC_Init(&NVIC_InitStructure);	//¸ù¾ÝÖ¸¶¨µÄ²ÎÊý³õÊ¼»¯VIC¼Ä´æÆ÷

	SPI_Cmd(SPI1,ENABLE);	//STM32Ê¹ÄÜSPI1
 // SPI1_ReadWriteByte(0xff);//Æô¶¯´«Êä		 
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
	
	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET) //¼ì²éÖ¸¶¨µÄSPI±êÖ¾Î»ÉèÖÃÓë·ñ:·¢ËÍ»º´æ¿Õ±êÖ¾Î»
		{
		 retry++;
		 if(retry>200)return 0;
		}			  
	   SPI_I2S_SendData(SPI1, TxData); //Í¨¹ýÍâÉèSPIx·¢ËÍÒ»¸öÊý¾Ý
	   retry=0;

	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET) //¼ì²éÖ¸¶¨µÄSPI±êÖ¾Î»ÉèÖÃÓë·ñ:½ÓÊÜ»º´æ·Ç¿Õ±êÖ¾Î»
		{
		 retry++;
		 if(retry>200)return 0;
		}	  						    
	return SPI_I2S_ReceiveData(SPI1); //·µ»ØÍ¨¹ýSPIx×î½ü½ÓÊÕµÄÊý¾Ý					    
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

	RCC_APB2PeriphClockCmd(	RCC_APB2Periph_GPIOB, ENABLE );//PORTBÊ±ÖÓÊ¹ÄÜ 
	RCC_APB1PeriphClockCmd(	RCC_APB1Periph_SPI2,  ENABLE );//SPI2Ê±ÖÓÊ¹ÄÜ 	
 
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;  //PB13/14/15¸´ÓÃÍÆÍìÊä³ö 
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);//³õÊ¼»¯GPIOB

 	GPIO_SetBits(GPIOB,GPIO_Pin_13|GPIO_Pin_14|GPIO_Pin_15);  //PB13/14/15ÉÏÀ­

	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;  //ÉèÖÃSPIµ¥Ïò»òÕßË«ÏòµÄÊý¾ÝÄ£Ê½:SPIÉèÖÃÎªË«ÏßË«ÏòÈ«Ë«¹¤
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;		//ÉèÖÃSPI¹¤×÷Ä£Ê½:ÉèÖÃÎªÖ÷SPI
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;		//ÉèÖÃSPIµÄÊý¾Ý´óÐ¡:SPI·¢ËÍ½ÓÊÕ8Î»Ö¡½á¹¹
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;		//´®ÐÐÍ¬²½Ê±ÖÓµÄ¿ÕÏÐ×´Ì¬Îª¸ßµçÆ½
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;	//´®ÐÐÍ¬²½Ê±ÖÓµÄµÚ¶þ¸öÌø±äÑØ£¨ÉÏÉý»òÏÂ½µ£©Êý¾Ý±»²ÉÑù
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;		//NSSÐÅºÅÓÉÓ²¼þ£¨NSS¹Ü½Å£©»¹ÊÇÈí¼þ£¨Ê¹ÓÃSSIÎ»£©¹ÜÀí:ÄÚ²¿NSSÐÅºÅÓÐSSIÎ»¿ØÖÆ
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8;		//¶¨Òå²¨ÌØÂÊÔ¤·ÖÆµµÄÖµ:²¨ÌØÂÊÔ¤·ÖÆµÖµÎª256
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;	//Ö¸¶¨Êý¾Ý´«Êä´ÓMSBÎ»»¹ÊÇLSBÎ»¿ªÊ¼:Êý¾Ý´«Êä´ÓMSBÎ»¿ªÊ¼
	SPI_InitStructure.SPI_CRCPolynomial = 7;	//CRCÖµ¼ÆËãµÄ¶àÏîÊ½
	SPI_Init(SPI2, &SPI_InitStructure);  //¸ù¾ÝSPI_InitStructÖÐÖ¸¶¨µÄ²ÎÊý³õÊ¼»¯ÍâÉèSPIx¼Ä´æÆ÷
 
	SPI_Cmd(SPI2, ENABLE); //Ê¹ÄÜSPIÍâÉè
	
	SPI2_ReadWriteByte(0xff);//Æô¶¯´«Êä		 SN74VC1T
 

}   

void SPI2_SetSpeed(u8 SPI_BaudRatePrescaler)
{
  assert_param(IS_SPI_BAUDRATE_PRESCALER(SPI_BaudRatePrescaler));
	SPI2->CR1&=0XFFC7;
	SPI2->CR1|=SPI_BaudRatePrescaler;	//ÉèÖÃSPI2ËÙ¶È 
	SPI_Cmd(SPI2,ENABLE); 

} 

//SPIx ¶ÁÐ´Ò»¸ö×Ö½Ú
//TxData:ÒªÐ´ÈëµÄ×Ö½Ú
//·µ»ØÖµ:¶ÁÈ¡µ½µÄ×Ö½Ú
u8 SPI2_ReadWriteByte(u8 TxData)
{		
	u8 retry=0;				 	
	while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE) == RESET) //¼ì²éÖ¸¶¨µÄSPI±êÖ¾Î»ÉèÖÃÓë·ñ:·¢ËÍ»º´æ¿Õ±êÖ¾Î»
		{
		retry++;
		if(retry>200)return 0;
		}			  
	SPI_I2S_SendData(SPI2, TxData); //Í¨¹ýÍâÉèSPIx·¢ËÍÒ»¸öÊý¾Ý
	retry=0;

//	while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_RXNE) == RESET) //¼ì²éÖ¸¶¨µÄSPI±êÖ¾Î»ÉèÖÃÓë·ñ:½ÓÊÜ»º´æ·Ç¿Õ±êÖ¾Î»
//		{
	//	retry++;
	//	if(retry>200)return 0;
//		}	  						    
	return SPI_I2S_ReceiveData(SPI2); //·µ»ØÍ¨¹ýSPIx×î½ü½ÓÊÕµÄÊý¾Ý					    
}

//=====================================================================================

//Á¬Ðø¶à×Ö½Ú·¢ËÍ
u8 SPI2_WriteByte(u8 * buff,u16 len)
{
	u8 retry=0;
	u8 TxData;
	u16 i;
	
	for(i=0;i<len;i++)
	{
		TxData=*(buff+i);
	 while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE) == RESET) //¼ì²éÖ¸¶¨µÄSPI±êÖ¾Î»ÉèÖÃÓë·ñ:·¢ËÍ»º´æ¿Õ±êÖ¾Î»
		{
		 retry++;
		 if(retry>200)return 0;
		}			  
	 SPI_I2S_SendData(SPI2, TxData); //Í¨¹ýÍâÉèSPIx·¢ËÍÒ»¸öÊý¾Ý
	 retry=0;
	}
	return 0;
}

//=============================================================================================
void SPI3_Init(void)
{
  GPIO_InitTypeDef    GPIO_InitStructure;
	SPI_InitTypeDef   	SPI_InitStructure;	
	
	RCC_APB2PeriphClockCmd(	RCC_APB2Periph_AFIO|RCC_APB2Periph_GPIOB, ENABLE );
	RCC_APB1PeriphClockCmd(	RCC_APB1Periph_SPI3,  ENABLE );//
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);//
	RCC_PCLK1Config(RCC_HCLK_Div2);                        //
	
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
//	GPIO_SetBits(GPIOB,GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7);

	/* 3?ê??¯????STM32 SPI1 */
	SPI_InitStructure.SPI_Direction=SPI_Direction_2Lines_FullDuplex;	//SPI
	SPI_InitStructure.SPI_Mode=SPI_Mode_Master;							//
	SPI_InitStructure.SPI_DataSize=SPI_DataSize_8b;						//
	SPI_InitStructure.SPI_CPOL=SPI_CPOL_High;							//
	SPI_InitStructure.SPI_CPHA=SPI_CPHA_2Edge;							//
	SPI_InitStructure.SPI_NSS=SPI_NSS_Soft;								//
//SPI_InitStructure.SPI_BaudRatePrescaler=SPI_BaudRatePrescaler_2;	//2
  SPI_InitStructure.SPI_BaudRatePrescaler=SPI_BaudRatePrescaler_2;
	SPI_InitStructure.SPI_FirstBit=SPI_FirstBit_MSB;					//
	SPI_InitStructure.SPI_CRCPolynomial=7;								//
	SPI_Init(SPI3,&SPI_InitStructure);									//
	
	
	SPI_Cmd(SPI3, ENABLE); //
		
}


void SPI3_SetSpeed(u8 SPI_BaudRatePrescaler)
{
  assert_param(IS_SPI_BAUDRATE_PRESCALER(SPI_BaudRatePrescaler));
	SPI3->CR1&=0XFFC7;
	SPI3->CR1|=SPI_BaudRatePrescaler;	// 
	SPI_Cmd(SPI3,ENABLE);
} 




//SPIx ¶ÁÐ´Ò»¸ö×Ö½Ú
//TxData:ÒªÐ´ÈëµÄ×Ö½Ú
//·µ»ØÖµ:¶ÁÈ¡µ½µÄ×Ö½Ú
u8 SPI3_ReadWriteByte(u8 TxData)
{		
	u8 retry=0;				 	
	while (SPI_I2S_GetFlagStatus(SPI3, SPI_I2S_FLAG_TXE) == RESET) //?????SPI???????:????????
	{
	retry++;
	if(retry>200)return 0;
	}			  
	SPI_I2S_SendData(SPI3, TxData); //????SPIx??????
	retry=0;

	while (SPI_I2S_GetFlagStatus(SPI3, SPI_I2S_FLAG_RXNE) == RESET) //?????SPI???????:?????????
	{
	retry++;
	if(retry>200)return 0;
	}	  						    
	return SPI_I2S_ReceiveData(SPI3); //????SPIx???????						    
}


//====================================================================================================






























//=====Ä£ÄâSPI¶ÁÐ´======================================================================


















