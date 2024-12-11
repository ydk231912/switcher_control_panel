#include "dma.h"
#include "led.h"
#include "usart.h"
#include "rgbw.h"
#include "fun.h"

volatile u8  ucByteBufRX1[DMARX1];    //����1ͨѶ����
volatile u8  ucByteBufTX1[DMATX1];

volatile u8  ucByteBufRX2[DMARX2];    //����1ͨѶ����
volatile u8  ucByteBufTX2[DMATX2];

volatile u8  ucByteBufRX3[DMARX3];    //����3ͨѶ����
volatile u8  ucByteBufTX3[DMATX3];
u16 DMA1_MEM_LEN;//����DMAÿ�����ݴ��͵ĳ��� 
u16 DMA1_MEM_LEN2;//����DMAÿ�����ݴ��͵ĳ��� 
//DMA1�ĸ�ͨ������
//����Ĵ�����ʽ�ǹ̶���,���Ҫ���ݲ�ͬ��������޸�
//�Ӵ洢��->����ģʽ/8λ���ݿ��/�洢������ģʽ
//DMA_CHx:DMAͨ��CHx
//cpar:�����ַ
//cmar:�洢����ַ
//cndtr:���ݴ����� 

u16 temp2;

void DMA_USART1_Tx_Config(DMA_Channel_TypeDef* DMA_CHx,u32 cpar,u32 cmar,u16 cndtr)
{

	NVIC_InitTypeDef NVIC_InitStructure;
 	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);	//ʹ��DMA����
	
  RCC->AHBENR|=1<<0;			//����DMA1ʱ��
//	delay_ms(500);			    //�ȴ�DMAʱ���ȶ�
	DMA_CHx->CPAR=cpar; 	 	//DMA1 �����ַ 
	DMA_CHx->CMAR=(u32)cmar; 	//DMA1,�洢����ַ
	DMA1_MEM_LEN=cndtr;      	//����DMA����������
	DMA_CHx->CNDTR=cndtr;    	//DMA1,����������
	DMA_CHx->CCR=0X00000000;	//��λ
	DMA_CHx->CCR|=1<<4;  		//�Ӵ洢����  
	DMA_CHx->CCR|=0<<5;  		//��ͨģʽ
	DMA_CHx->CCR|=0<<6; 		//�����ַ������ģʽ
	DMA_CHx->CCR|=1<<7; 	 	//�洢������ģʽ
	DMA_CHx->CCR|=0<<8; 	 	//�������ݿ��Ϊ8λ
	DMA_CHx->CCR|=0<<10; 		//�洢�����ݿ��8λ
	DMA_CHx->CCR|=0<<12; 		//�е����ȼ�
	DMA_CHx->CCR|=0<<14; 		//�Ǵ洢�����洢��ģʽ
	DMA_CHx->CCR|=1<<1;			//����������ж�
	
	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel4_IRQn;  //����DMA�ж�
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3 ;//��ռ���ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		//�����ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨ��ʹ��
	NVIC_Init(&NVIC_InitStructure);	//����ָ���Ĳ�����ʼ��VIC�Ĵ���
}  
//==============================================================

void DMA_USART1_Rx_Config(DMA_Channel_TypeDef* DMA_CHx,u32 cpar,u32 cmar,u16 cndtr)
{
	
	NVIC_InitTypeDef NVIC_InitStructure;
 	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);	//ʹ��DMA����
	
  RCC->AHBENR|=1<<0;			//����DMA1ʱ��
//	delay_ms(500);			    //�ȴ�DMAʱ���ȶ�
	DMA_CHx->CPAR=cpar; 	 	//DMA1 �����ַ 
	DMA_CHx->CMAR=(u32)cmar; 	//DMA1,�洢����ַ
	DMA1_MEM_LEN=cndtr;      	//����DMA����������
	DMA_CHx->CNDTR=cndtr;    	//DMA1,����������
	DMA_CHx->CCR=0X00000000;	//��λ
	DMA_CHx->CCR|=1<<1;			//����������ж�
	DMA_CHx->CCR|=0<<4;  		//�������
	DMA_CHx->CCR|=0<<5;  		//��ͨģʽ
	DMA_CHx->CCR|=0<<6; 		//�����ַ������ģʽ
	DMA_CHx->CCR|=1<<7; 	 	//�洢������ģʽ
	DMA_CHx->CCR|=0<<8; 	 	//�������ݿ��Ϊ8λ
	DMA_CHx->CCR|=0<<10; 		//�洢�����ݿ��8λ
	DMA_CHx->CCR|=1<<12; 		//�е����ȼ�
	DMA_CHx->CCR|=0<<14; 		//�Ǵ洢�����洢��ģʽ
 	USART1->CR3|=1<<6;			//����USART1DMA����
	DMA_CHx->CCR|=1<<0;			//Ĭ�Ͽ�������
	DMA_CHx->CCR|=1<<1;			//����������ж�
 
	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel5_IRQn;  //����DMA�ж�
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3 ;//��ռ���ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		//�����ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨ��ʹ��
	NVIC_Init(&NVIC_InitStructure);	//����ָ���Ĳ�����ʼ��VIC�Ĵ���	
}

//==========================================================
//����һ��DMA����
void DMA_USART1_Tx(DMA_Channel_TypeDef*DMA_CHx,u16 len)
{ 
	USART1->CR3=1<<7;			   //��������1DMA
	DMA1->IFCR|=1<<13;          //���ͨ��4������ɱ�־ 
	DMA_CHx->CCR&=~(1<<0);       //�ر�DMA���� 
	DMA_CHx->CNDTR=len; //DMA1,���������� 		  
	DMA_CHx->CCR|=1<<0;          //����DMA����
	USART1->CR3|=1<<6;			   //��ǰ�򿪴���ͨѶ  ��ֹ�����ӳ�
	
}	  
//===================================================================
 
void DMA1_Channel4_IRQHandler (void)
{
	DMA1->IFCR|=1<<13;     //���ͨ��4������ɱ�־
	DMA1->IFCR|=1<<12;     ////���ͨ��4ȫ�ִ�����ɱ�־
  USART1->CR3&=~(1<<7);	 //�رմ��ڷ���DMA 
	USART1_TxOV++;
}
//===================================================================
void DMA1_Channel5_IRQHandler(void)	//������ж�//���DMA�ж�
{
   DMA1->IFCR|=1<<17;     //���ͨ��4������ɱ�־ 
	
	 if(DMA_GetITStatus(DMA1_IT_TC5))
		 {
       DMA_Cmd(DMA1_Channel5, DISABLE);
       DMA_ClearITPendingBit(DMA1_IT_TC5); //���ж�	 
     }
}



//============================================================================

//==============================================================================

void DMA_USART2_Tx_Config(DMA_Channel_TypeDef* DMA_CHx,u32 cpar,u32 cmar,u16 cndtr)
{
	NVIC_InitTypeDef NVIC_InitStructure;
 	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);	//ʹ��DMA����
	
  RCC->AHBENR|=1<<0;			//����DMA1ʱ��
//	delay_ms(500);			//�ȴ�DMAʱ���ȶ�
	DMA_CHx->CPAR=cpar; 	 	//DMA1 �����ַ 
	DMA_CHx->CMAR=(u32)cmar; 	//DMA1,�洢����ַ
	DMA1_MEM_LEN2=cndtr;      	//����DMA����������
	DMA_CHx->CNDTR=cndtr;    	//DMA1,����������
	DMA_CHx->CCR=0X00000000;	//��λ
	DMA_CHx->CCR|=1<<4;  		//�Ӵ洢����  
	DMA_CHx->CCR|=0<<5;  		//��ͨģʽ
	DMA_CHx->CCR|=0<<6; 		//�����ַ������ģʽ
	DMA_CHx->CCR|=1<<7; 	 	//�洢������ģʽ
	DMA_CHx->CCR|=0<<8; 	 	//�������ݿ��Ϊ8λ
	DMA_CHx->CCR|=0<<10; 		//�洢�����ݿ��8λ
	DMA_CHx->CCR|=0<<12; 		//�е����ȼ�
	DMA_CHx->CCR|=0<<14; 		//�Ǵ洢�����洢��ģʽ
	DMA_CHx->CCR|=1<<1;			//����������ж�
	
	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel7_IRQn;  //����DMA�ж�
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=2 ;//��ռ���ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;		//�����ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨ��ʹ��
	NVIC_Init(&NVIC_InitStructure);	    //����ָ���Ĳ�����ʼ��VIC�Ĵ���
}  
//=====================================================================================

void DMA_USART2_Rx_Config(DMA_Channel_TypeDef* DMA_CHx,u32 cpar,u32 cmar,u16 cndtr)
{
	
	NVIC_InitTypeDef NVIC_InitStructure;
 	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);	//ʹ��DMA����
	
  RCC->AHBENR|=1<<0;			//����DMA1ʱ��
//	delay_ms(500);			    //�ȴ�DMAʱ���ȶ�
	DMA_CHx->CPAR=cpar; 	 	//DMA1 �����ַ 
	DMA_CHx->CMAR=(u32)cmar; 	//DMA1,�洢����ַ
	DMA1_MEM_LEN2=cndtr;      	//����DMA����������
	DMA_CHx->CNDTR=cndtr;    	//DMA1,����������
	DMA_CHx->CCR=0X00000000;	//��λ
	DMA_CHx->CCR|=1<<1;			//����������ж�
	DMA_CHx->CCR|=0<<4;  		//�������
	DMA_CHx->CCR|=0<<5;  		//��ͨģʽ
	DMA_CHx->CCR|=0<<6; 		//�����ַ������ģʽ
	DMA_CHx->CCR|=1<<7; 	 	//�洢������ģʽ
	DMA_CHx->CCR|=0<<8; 	 	//�������ݿ��Ϊ8λ
	DMA_CHx->CCR|=0<<10; 		//�洢�����ݿ��8λ
	DMA_CHx->CCR|=1<<12; 		//�е����ȼ�
	DMA_CHx->CCR|=0<<14; 		//�Ǵ洢�����洢��ģʽ
	USART2->CR3|=1<<6;			//����USART2DMA����
	DMA_CHx->CCR|=1<<0;			//Ĭ�Ͽ�������
 
	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel6_IRQn;  //����DMA�ж�
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3 ;//��ռ���ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		//�����ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨ��ʹ��
	NVIC_Init(&NVIC_InitStructure);	//����ָ���Ĳ�����ʼ��VIC�Ĵ���	
}
//===================================================================================
//����һ��DMA����
void DMA_USART2_Tx(DMA_Channel_TypeDef*DMA_CHx,u16 len)
{ 
	USART2->CR3=1<<7;			   //ʹ�ܷ���ʱ��DMAģʽ
	DMA1->IFCR|=1<<25;             //���ͨ��4������ɱ�־ 
	DMA_CHx->CCR&=~(1<<0);         //�ر�DMA���� 
	DMA_CHx->CNDTR=len; //DMA1,���������� 		  
	DMA_CHx->CCR|=1<<0;            //����DMA����
	USART2->CR3|=1<<6;			   //��ǰ�򿪴���ͨѶ  ��ֹ�����ӳ� 

}	  

 
void DMA1_Channel7_IRQHandler (void)
{
	DMA1->IFCR|=1<<25;     //���ͨ��4������ɱ�־
	DMA1->IFCR|=1<<24;
  USART2->CR3&=~(1<<7);	 //�رմ��ڷ���DMA   
  
}


//======================================================================================


//DMA�����ж����ڹ̶����ݣ�һ�㲻��Ӧ���ر�DMA�����ж�
void DMA1_Channel6_IRQHandler (void)
{
 if(DMA_GetITStatus(DMA1_IT_TC6) != RESET)
 {
  DMA1->IFCR|=1<<17;     //���ͨ��4������ɱ�־
 }
}
//=========================================================================================

//==============================================================================

void DMA_USART3_Tx_Config(DMA_Channel_TypeDef* DMA_CHx,u32 cpar,u32 cmar,u16 cndtr)
{
	
	NVIC_InitTypeDef NVIC_InitStructure;
 	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);	//ʹ��DMA����
	
  RCC->AHBENR|=1<<0;			//����DMA1ʱ��
 //	delay_ms(500);			//�ȴ�DMAʱ���ȶ�
	DMA_CHx->CPAR=cpar; 	 	//DMA1 �����ַ 
	DMA_CHx->CMAR=(u32)cmar; 	//DMA1,�洢����ַ
	DMA1_MEM_LEN2=cndtr;      	//����DMA����������
	DMA_CHx->CNDTR=cndtr;    	//DMA1,����������
	DMA_CHx->CCR=0X00000000;	//��λ
	DMA_CHx->CCR|=1<<4;  		//�Ӵ洢����  
	DMA_CHx->CCR|=0<<5;  		//��ͨģʽ
	DMA_CHx->CCR|=0<<6; 		//�����ַ������ģʽ
	DMA_CHx->CCR|=1<<7; 	 	//�洢������ģʽ
	DMA_CHx->CCR|=0<<8; 	 	//�������ݿ��Ϊ8λ
	DMA_CHx->CCR|=0<<10; 		//�洢�����ݿ��8λ
	DMA_CHx->CCR|=0<<12; 		//�е����ȼ�
	DMA_CHx->CCR|=0<<14; 		//�Ǵ洢�����洢��ģʽ
	DMA_CHx->CCR|=1<<1;			//����������ж�

	
	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel2_IRQn;  //����DMA�ж�
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3 ;//��ռ���ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		//�����ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨ��ʹ��
	NVIC_Init(&NVIC_InitStructure);	    //����ָ���Ĳ�����ʼ��VIC�Ĵ���
}  

//===================================================================================
//����һ��DMA����
void DMA_USART3_Tx(DMA_Channel_TypeDef*DMA_CHx,u16 len)
{ 
	USART3->CR3=1<<7;			   //ʹ�ܷ���ʱ��DMAģʽ
	DMA1->IFCR|=1<<5;             //���ͨ��4������ɱ�־ 
	DMA_CHx->CCR&=~(1<<0);         //�ر�DMA���� 
	DMA_CHx->CNDTR=len;        //DMA1,���������� 		  
	DMA_CHx->CCR|=1<<0;            //����DMA����
	USART3->CR3|=1<<6;			   //��ǰ�򿪴���ͨѶ  ��ֹ�����ӳ�  

}	  

 
void DMA1_Channel2_IRQHandler (void)
{
	DMA1->IFCR|=1<<5;     //���ͨ��4������ɱ�־
	DMA1->IFCR|=1<<9;
  USART3->CR3&=~(1<<7);	 //�رմ��ڷ���DMA   
  USART3_TxOV++;
//	LED2=!LED2;
}
//=====================================================================================

void DMA_USART3_Rx_Config(DMA_Channel_TypeDef* DMA_CHx,u32 cpar,u32 cmar,u16 cndtr)
{
	
	
	NVIC_InitTypeDef NVIC_InitStructure;
 	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);	//ʹ��DMA����
	
  RCC->AHBENR|=1<<0;			//����DMA1ʱ��
//	delay_ms(500);			    //�ȴ�DMAʱ���ȶ�
	DMA_CHx->CPAR=cpar; 	 	//DMA1 �����ַ 
	DMA_CHx->CMAR=(u32)cmar; 	//DMA1,�洢����ַ
	DMA1_MEM_LEN2=cndtr;      	//����DMA����������
	DMA_CHx->CNDTR=cndtr;    	//DMA1,����������
	DMA_CHx->CCR=0X00000000;	//��λ
	DMA_CHx->CCR|=1<<1;			//����������ж�
	DMA_CHx->CCR|=0<<4;  		//�������
	DMA_CHx->CCR|=0<<5;  		//��ͨģʽ
	DMA_CHx->CCR|=0<<6; 		//�����ַ������ģʽ
	DMA_CHx->CCR|=1<<7; 	 	//�洢������ģʽ
	DMA_CHx->CCR|=0<<8; 	 	//�������ݿ��Ϊ8λ
	DMA_CHx->CCR|=0<<10; 		//�洢�����ݿ��8λ
	DMA_CHx->CCR|=1<<12; 		//�е����ȼ�
	DMA_CHx->CCR|=0<<14; 		//�Ǵ洢�����洢��ģʽ
	USART3->CR3|=1<<6;			//����USART2DMA����
	DMA_CHx->CCR|=1<<0;			//Ĭ�Ͽ�������
 
	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel3_IRQn;  //����DMA�ж�
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3 ;//��ռ���ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		//�����ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨ��ʹ��
	NVIC_Init(&NVIC_InitStructure);	//����ָ���Ĳ�����ʼ��VIC�Ĵ���	
}

//======================================================================================


//DMA�����ж����ڹ̶����ݣ�һ�㲻��Ӧ���ر�DMA�����ж�
void DMA1_Channel3_IRQHandler (void)
{
 if(DMA_GetITStatus(DMA1_IT_TC3) != RESET)
 {
	DMA_ClearITPendingBit(DMA1_IT_TC3);
  DMA_ClearFlag(DMA1_FLAG_TC3); //���DMA�жϱ�־
//USART_ClearFlag(USART1,USART_FLAG_TC); //��������жϱ�־
//DMA_Cmd(DMA1_Channel5, DISABLE );      //�ر�DMA����
 }
}//==========================================================================================

void DMA_SPI2_Tx_Config(void)
{
	 DMA_InitTypeDef  DMA_InitStructure;
	 NVIC_InitTypeDef NVIC_InitStructure;
 	 RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);	//ʹ��DMA����
	
	
   DMA_DeInit(DMA1_Channel5);
   DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)(&SPI2->DR); //DMA����ADC����ַ
   DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)gWs2812bDat_SPI;  //DMA�ڴ����ַ
   DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;       //���ݴ��䷽�򣬴��ڴ��ȡ���͵�����
   DMA_InitStructure.DMA_BufferSize = WS2812B_AMOUNT * 32;  //DMAͨ����DMA����Ĵ�С
   DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;   //�����ַ�Ĵ�������
   DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;            //�ڴ��ַ�Ĵ�������  
   DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;   //���ݿ��Ϊ8λ
   DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;            //���ݿ��Ϊ8λ
   DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;                       //��������������ģʽ
   DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;         //DMAͨ�� xӵ�������ȼ� 
   DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;                 //DMAͨ��xû������Ϊ�ڴ浽�ڴ洫��   
   DMA_Init(DMA1_Channel5, &DMA_InitStructure);  

	if(DMA_GetITStatus(DMA1_IT_TC5)!= RESET)
		{
     DMA_ClearITPendingBit(DMA1_IT_TC5);
    }
	
		
	DMA_ITConfig(DMA1_Channel5, DMA_IT_TC, ENABLE);
  SPI_I2S_DMACmd(SPI2, SPI_I2S_DMAReq_Tx, ENABLE);
  DMA_Cmd(DMA1_Channel5, DISABLE);
	
	
	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel5_IRQn;  //����DMA�ж�
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3 ;//��ռ���ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		//�����ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨ��ʹ��
	NVIC_Init(&NVIC_InitStructure);	    //����ָ���Ĳ�����ʼ��VIC�Ĵ���
}  

//===================================================================================
//����һ��DMA����
void DMA_SPI2_Tx(u8 *buff,u16 LEN)
{ 

	 DMA_Cmd(DMA1_Channel5,DISABLE);
   DMA1_Channel5->CNDTR = LEN;
	 DMA1_Channel5->CMAR=(u32)buff;
   DMA_Cmd(DMA1_Channel5,ENABLE);
   
	
	
}	  


