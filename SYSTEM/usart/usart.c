#include "sys.h"
#include "usart.h"	  
#include "led.h"
#include "dma.h"
//////////////////////////////////////////////////////////////////


 //=========================================================================================
u16 USART1_RxLen=0;   //���ڽ��յ�����������
u16 USART2_RxLen=0;
u8 USART1_RxOV,USART1_TxOV; //����1������ɱ�־
u8 USART2_RxOV,USART2_TxOV; //����2������ɱ�־
u8 USART3_RxOV,USART3_TxOV; //����2������ɱ�־
u16 Uart1_tempreset,Uart2_tempreset,Uart3_tempreset;

void uart1_init(u32 pclk2,u32 bound){
	
		
  float temp;
	u16 mantissa;
	u16 fraction;	 

	NVIC_InitTypeDef NVIC_InitStructure;

  temp=(float)(pclk2*1000000)/(bound*16);//�õ�USARTDIV
	mantissa=temp;				 //�õ���������
	fraction=(temp-mantissa)*16; //�õ�С������	 
  mantissa<<=4;
	mantissa+=fraction; 

	RCC->APB2ENR|=1<<2;   //ʹ��PORTA��ʱ��  
	RCC->APB2ENR|=1<<14;  //ʹ�ܴ���ʱ�� 
	GPIOA->CRH&=0XFFFFF00F; 
	GPIOA->CRH|=0X000008B0;//IO״̬����
		  
	RCC->APB2RSTR|=1<<14;   //��λ����1
	RCC->APB2RSTR&=~(1<<14);//ֹͣ��λ	   	   
 	USART1->BRR=mantissa; // ����������		 
	USART1->CR1|=0X200C;  //1λֹͣ,��У��λ.

	USART1->CR1|=1<<8;    //PE�ж�ʹ��	����USART_SR�е�PEΪ��1��ʱ������USART�жϡ�
//USART1->CR1|=1<<5;    //���ջ������ǿ��ж�ʹ�� ��USART_SR�е�ORE����RXNEΪ��1��ʱ������USART�жϡ�
	USART1->CR1|=1<<4;	  //��USART_SR�е�IDLEΪ��1��ʱ������USART�ж�

//USART1->CR1|=1<<6;    //��USART_SR�е�TCΪ��1��ʱ������USART�ж�
//USART1->CR1|=1<<7;	   //��USART_SR�е�TXEΪ��1��ʱ������USART�ж�    	

	USART1->SR&=~0x0010;
	
   //Usart1 NVIC ����
  NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3 ;//��ռ���ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		//�����ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨ��ʹ��
	NVIC_Init(&NVIC_InitStructure);	//����ָ���Ĳ�����ʼ��VIC�Ĵ���


}

void USART1_IRQHandler(void)                	//����1�жϷ������
	{
	 if(USART_GetITStatus(USART1, USART_IT_IDLE) != RESET)  //�����ж�
		{
		 if(((USART1->SR&0x0040)==0x0040))
	 {	 	
	 	 USART1->CR1&=~0x0040;	    						 	  		
	 }		 
	if((USART1->SR&0x0020)==0x0020)  USART1->SR&=~0x0020; 	    //��������жϱ�־λ	
	 
	  if((USART1->SR&0x0010)==0x0010)  //����⵽���߿���ʱ����λ��Ӳ����λ�����USART_CR1�е�IDLEIEΪ��1����������ж�
		 {
			USART1->SR&=~0x0010; 
		  Uart1_tempreset=USART1->SR;
		  Uart1_tempreset=USART1->DR;
		  
		  DMA1_Channel5->CCR&=~(1<<0);   //�ر�DMA����
		  USART1->CR3&=~(1<<6);			     //�ر�USART1DMA����	  	  
	 // Uart1CountRx=DMARX1-DMA1_Channel5->CNDTR;   //���ʣ�ഫ����  						               		  			
		  DMA1_Channel5->CNDTR=DMARX1C;	   //��������   ע��Ҫ�������Ӧ
		  DMA1_Channel5->CCR|=(1<<0);		   //����DMA����  
		  USART1->CR3|=1<<6;				       //��������DMA����
		  DMA1->IFCR|=1<<17;				       //���DMA�ж�
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

 temp=(float)(pclk2*1000000)/(bound*16);//�õ�USARTDIV  da_taa=;
	mantissa=temp;				 //�õ���������
	fraction=(temp-mantissa)*16; //�õ�С������	 
  mantissa<<=4;
	mantissa+=fraction; 

	RCC->APB2ENR|=1<<2;   //ʹ��PORTA��ʱ�� 	 
	RCC->APB1ENR|=1<<17;  //ʹ�ܴ���ʱ�� 

	GPIOA->CRL&=0XFFFF00FF; 
	GPIOA->CRL|=0X00008B00;  //IO״̬����
		  
	RCC->APB1RSTR|=1<<17;   //��λ����1
	RCC->APB1RSTR&=~(1<<17);//ֹͣ��λ	   	   

 	USART2->BRR=mantissa; // ����������		 
	USART2->CR1|=0X200C;  //1λֹͣ,��У��λ.

	USART2->CR1|=1<<8;    //PE�ж�ʹ��	����USART_SR�е�PEΪ��1��ʱ������USART�жϡ�
//USART1->CR1|=1<<5;    //���ջ������ǿ��ж�ʹ�� ��USART_SR�е�ORE����RXNEΪ��1��ʱ������USART�жϡ�
	USART2->CR1|=1<<4;	  //��USART_SR�е�IDLEΪ��1��ʱ������USART�ж�

//	USART1->CR1|=1<<6;    //��USART_SR�е�TCΪ��1��ʱ������USART�ж�
//	USART1->CR1|=1<<7;	   //��USART_SR�е�TXEΪ��1��ʱ������USART�ж�
    	
	Uart2_tempreset=USART2->SR;		 //�ϵ縴λ�ж�
	Uart2_tempreset=USART2->DR;
	USART2->SR&=~0x0010;	   
	
//Usart1 NVIC ����
  NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3 ;//��ռ���ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		//�����ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨ��ʹ��
	NVIC_Init(&NVIC_InitStructure);	//����ָ���Ĳ�����ʼ��VIC�Ĵ���


}

void USART2_IRQHandler(void)                	//����1�жϷ������
	{
	 if(((USART2->SR&0x0040)==0x0040))
	 {	 	
	 	 USART2->CR1&=~0x0040;
     }

	 if((USART2->SR&0x0010)==0x0010)      //����⵽���߿���ʱ����λ��Ӳ����λ�����USART_CR1�е�IDLEIEΪ��1����������ж�
		 {
		  USART2->SR&=~0x0010;
		 								  //�������׸��ţ�
		  Uart2_tempreset=USART2->SR;
		  Uart2_tempreset=USART2->DR;
		  
		  DMA1_Channel6->CCR&=~(1<<0);       //�ر�DMA����
		  USART2->CR3&=~(1<<6);			     //�ر�USART1DMA����	
		  	  
//		ModUartCountRx2=DMARX2-DMA1_Channel6->CNDTR;   //���ʣ�ഫ����
//		UartRxTimerStartFlag2=1;						       //������ɱ�־	
		  	  			
		  DMA1_Channel6->CNDTR=DMARX2;			//��������
		  DMA1_Channel6->CCR|=(1<<0);			//����DMA����  
		  USART2->CR3|=1<<6;				    //��������DMA����
		  DMA1->IFCR|=1<<21;					//���DMA�ж�
		  DMA1->IFCR|=1<<25;			 	 	  	  
		  }	

		  USART2->SR&=~0x0010;
		  USART2->CR1&=~0x0040;	

      USART2_RxOV++;
  } 

	
	//=========================================================================================
	//=================================================================================

void uart3_init(u32 pclk2,u32 bound)
	{
		float temp;
	u16 mantissa;
	u16 fraction;	

	NVIC_InitTypeDef NVIC_InitStructure;

  //USART ��ʼ������
	temp=(float)(pclk2*1000000)/(bound*16);//�õ�USARTDIV  da_taa=;
	mantissa=temp;				 //�õ���������
	fraction=(temp-mantissa)*16; //�õ�С������	 
  mantissa<<=4;
	mantissa+=fraction; 

	RCC->APB2ENR|=1<<3;   //ʹ��PORTA��ʱ�� 	 
	RCC->APB1ENR|=1<<18;  //ʹ�ܴ���ʱ�� 

	GPIOB->CRH&=0XFFFF00FF; 
	GPIOB->CRH|=0X00008B00;  //IO״̬����
		  
	RCC->APB1RSTR|=1<<18;   //��λ����1
	RCC->APB1RSTR&=~(1<<18);//ֹͣ��λ	   	   

 	USART3->BRR=mantissa; // ����������		 
	USART3->CR1|=0X200C;  //1λֹͣ,��У��λ.

	USART3->CR1|=1<<8;    //PE�ж�ʹ��	����USART_SR�е�PEΪ��1��ʱ������USART�жϡ�
//USART1->CR1|=1<<5;    //���ջ������ǿ��ж�ʹ�� ��USART_SR�е�ORE����RXNEΪ��1��ʱ������USART�жϡ�
	USART3->CR1|=1<<4;	  //��USART_SR�е�IDLEΪ��1��ʱ������USART�ж�

//	USART1->CR1|=1<<6;    //��USART_SR�е�TCΪ��1��ʱ������USART�ж�
//	USART1->CR1|=1<<7;	   //��USART_SR�е�TXEΪ��1��ʱ������USART�ж�
    	
	Uart3_tempreset=USART3->SR;		 //�ϵ縴λ�ж�
	Uart3_tempreset=USART3->DR;
	USART3->SR&=~0x0010;	   
	
  NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3 ;//��ռ���ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		//�����ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨ��ʹ��
	NVIC_Init(&NVIC_InitStructure);	//����ָ���Ĳ�����ʼ��VIC�Ĵ���


}

void USART3_IRQHandler(void)                	//����1�жϷ������
	{
	 if(((USART3->SR&0x0040)==0x0040))
	 {	 	
	 	 USART3->CR1&=~0x0040;
     }

	 if((USART3->SR&0x0010)==0x0010)      //����⵽���߿���ʱ����λ��Ӳ����λ�����USART_CR1�е�IDLEIEΪ��1����������ж�
		 {
		  USART3->SR&=~0x0010;
		 								  
		  Uart3_tempreset=USART3->SR;
		  Uart3_tempreset=USART3->DR;
		  
		  DMA1_Channel3->CCR&=~(1<<0);       //�ر�DMA����
		  USART3->CR3&=~(1<<6);			     //�ر�USART1DMA����	
		  	  
//		ModUartCountRx2=DMARX2-DMA1_Channel6->CNDTR;   //���ʣ�ഫ����
//		UartRxTimerStartFlag2=1;						       //������ɱ�־	
		  	  			
		  DMA1_Channel3->CNDTR=DMARX3;			//��������
		  DMA1_Channel3->CCR|=(1<<0);			//����DMA����  
		  USART3->CR3|=1<<6;				    //��������DMA����
		  DMA1->IFCR|=1<<5;					//���DMA�ж�
		  DMA1->IFCR|=1<<9;			 	 	  	  
		  }	

		  USART3->SR&=~0x0010;
		  USART3->CR1&=~0x0040;	

      USART3_RxOV++;
  } 










	
	
	
	
	
	
	






