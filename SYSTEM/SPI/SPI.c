#include "spi.h"
#include "delay.h"
#include "fun.h"
#include "led.h"
u8 SPI_RW(u8 buchar);	

u8 SPIbuf1[10];
u8 SPIbuf2[10];
u8 SPI1_ReadWriteByte(u8 TxData);
//�ӻ�ģʽ
void SPI1_Init(void)
{
	GPIO_InitTypeDef 	GPIO_InitStructure;
	SPI_InitTypeDef   	SPI_InitStructure;	   
  NVIC_InitTypeDef NVIC_InitStructure;
	
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_SPI1 | RCC_APB2Periph_AFIO, ENABLE);	

	/* ��ʼ��SCK��MISO��MOSI���� */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_SetBits(GPIOA,GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7);

	/* ��ʼ��CS���� */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_Out_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_SetBits(GPIOA, GPIO_Pin_11);

	/* ��ʼ������STM32 SPI1 */
	
	SPI_InitStructure.SPI_BaudRatePrescaler=SPI_BaudRatePrescaler_8;
	SPI_InitStructure.SPI_CPHA=SPI_CPHA_2Edge;							//���ݲ����ڵ�1��ʱ����
	SPI_InitStructure.SPI_CPOL=SPI_CPOL_High;							//ʱ�����յ�
	SPI_InitStructure.SPI_CRCPolynomial=7;								 //CRC����ʽΪ7
	SPI_InitStructure.SPI_DataSize=SPI_DataSize_8b;						//SPI���ͽ���8λ֡�ṹ
	SPI_InitStructure.SPI_Direction=SPI_Direction_2Lines_FullDuplex;	//SPI����Ϊ˫��˫��ȫ˫��
	SPI_InitStructure.SPI_Mode=SPI_Mode_Slave ;						//����Ϊ��SPI
	SPI_InitStructure.SPI_FirstBit=SPI_FirstBit_MSB;			 //���ݴ����MSBλ��ʼ
	SPI_InitStructure.SPI_NSS=SPI_NSS_Soft ;								//NSS���ⲿ�ܽŹ���
	
	SPI_I2S_ITConfig(SPI1,SPI_I2S_IT_RXNE,ENABLE);   //���������ж�
	SPI_Init(SPI1,&SPI_InitStructure);							 //����SPI_InitStruct��ָ���Ĳ�����ʼ������SPI1�Ĵ���
	
	NVIC_InitStructure.NVIC_IRQChannel = SPI1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3 ;//��ռ���ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		//�����ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨ��ʹ��
	NVIC_Init(&NVIC_InitStructure);	//����ָ���Ĳ�����ʼ��VIC�Ĵ���

	SPI_Cmd(SPI1,ENABLE);	//STM32ʹ��SPI1
 // SPI1_ReadWriteByte(0xff);//��������		 
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
	
	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET) //���ָ����SPI��־λ�������:���ͻ���ձ�־λ
		{
		 retry++;
		 if(retry>200)return 0;
		}			  
	   SPI_I2S_SendData(SPI1, TxData); //ͨ������SPIx����һ������
	   retry=0;

	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET) //���ָ����SPI��־λ�������:���ܻ���ǿձ�־λ
		{
		 retry++;
		 if(retry>200)return 0;
		}	  						    
	return SPI_I2S_ReceiveData(SPI1); //����ͨ��SPIx������յ�����					    
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

	RCC_APB2PeriphClockCmd(	RCC_APB2Periph_GPIOB, ENABLE );//PORTBʱ��ʹ�� 
	RCC_APB1PeriphClockCmd(	RCC_APB1Periph_SPI2,  ENABLE );//SPI2ʱ��ʹ�� 	
 
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;  //PB13/14/15����������� 
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);//��ʼ��GPIOB

 	GPIO_SetBits(GPIOB,GPIO_Pin_13|GPIO_Pin_14|GPIO_Pin_15);  //PB13/14/15����

	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;  //����SPI�������˫�������ģʽ:SPI����Ϊ˫��˫��ȫ˫��
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;		//����SPI����ģʽ:����Ϊ��SPI
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;		//����SPI�����ݴ�С:SPI���ͽ���8λ֡�ṹ
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;		//����ͬ��ʱ�ӵĿ���״̬Ϊ�ߵ�ƽ
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;	//����ͬ��ʱ�ӵĵڶ��������أ��������½������ݱ�����
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;		//NSS�ź���Ӳ����NSS�ܽţ����������ʹ��SSIλ������:�ڲ�NSS�ź���SSIλ����
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8;		//���岨����Ԥ��Ƶ��ֵ:������Ԥ��ƵֵΪ256
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;	//ָ�����ݴ����MSBλ����LSBλ��ʼ:���ݴ����MSBλ��ʼ
	SPI_InitStructure.SPI_CRCPolynomial = 7;	//CRCֵ����Ķ���ʽ
	SPI_Init(SPI2, &SPI_InitStructure);  //����SPI_InitStruct��ָ���Ĳ�����ʼ������SPIx�Ĵ���
 
	SPI_Cmd(SPI2, ENABLE); //ʹ��SPI����
	
	SPI2_ReadWriteByte(0xff);//��������		 SN74VC1T
 

}   

void SPI2_SetSpeed(u8 SPI_BaudRatePrescaler)
{
  assert_param(IS_SPI_BAUDRATE_PRESCALER(SPI_BaudRatePrescaler));
	SPI2->CR1&=0XFFC7;
	SPI2->CR1|=SPI_BaudRatePrescaler;	//����SPI2�ٶ� 
	SPI_Cmd(SPI2,ENABLE); 

} 

//SPIx ��дһ���ֽ�
//TxData:Ҫд����ֽ�
//����ֵ:��ȡ�����ֽ�
u8 SPI2_ReadWriteByte(u8 TxData)
{		
	u8 retry=0;				 	
	while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE) == RESET) //���ָ����SPI��־λ�������:���ͻ���ձ�־λ
		{
		retry++;
		if(retry>200)return 0;
		}			  
	SPI_I2S_SendData(SPI2, TxData); //ͨ������SPIx����һ������
	retry=0;

//	while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_RXNE) == RESET) //���ָ����SPI��־λ�������:���ܻ���ǿձ�־λ
//		{
	//	retry++;
	//	if(retry>200)return 0;
//		}	  						    
	return SPI_I2S_ReceiveData(SPI2); //����ͨ��SPIx������յ�����					    
}

//=====================================================================================

//�������ֽڷ���
u8 SPI2_WriteByte(u8 * buff,u16 len)
{
	u8 retry=0;
	u8 TxData;
	u16 i;
	
	for(i=0;i<len;i++)
	{
		TxData=*(buff+i);
	 while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE) == RESET) //���ָ����SPI��־λ�������:���ͻ���ձ�־λ
		{
		 retry++;
		 if(retry>200)return 0;
		}			  
	 SPI_I2S_SendData(SPI2, TxData); //ͨ������SPIx����һ������
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

	/* 3?�??�????STM32 SPI1 */
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




//SPIx ��дһ���ֽ�
//TxData:Ҫд����ֽ�
//����ֵ:��ȡ�����ֽ�
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






























//=====ģ��SPI��д======================================================================


















