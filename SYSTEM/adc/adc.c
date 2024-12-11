#include "sys.h"
#include "adc.h"
#include "led.h"
//#include "tim.h"


u32  ADC_tempdata1;	  //ADCת��
u16  ADC_out1;
u16  ADC_cnt1;
u16  ADCsim1;    //���ģ��ֵ
u8 ADC_Ch_cnt;   //ͨ��ת������
float adctemp1=0,adctemp2=0;

 void  Adc_Init(void)	   //173
{    

	//�ȳ�ʼ��IO��  /ADC12_IN5 6 7 8 9
	
 	RCC->APB2ENR|=1<<4;    //ʹ��PORTB��ʱ�� 
	GPIOC->CRL&=0XFFFFFFF0;  
	 
	RCC->APB2ENR|=1<<9;    //ADC1ʱ��ʹ��	  
	RCC->APB2RSTR|=1<<9;   //ADC1��λ
	RCC->APB2RSTR&=~(1<<9);//��λ����	    
	RCC->CFGR&=~(3<<14);   //��Ƶ��������	

	RCC->CFGR|=2<<14;      	 
	ADC1->CR1&=0XF0FFFF;   //����ģʽ����
	ADC1->CR1|=0<<16;      //��������ģʽ  
	ADC1->CR1&=~(1<<8);    //��ɨ��ģʽ	
	  
	ADC1->CR2&=~(1<<1);    //����ת��ģʽ
	ADC1->CR2&=~(7<<17);	   
	ADC1->CR2|=7<<17;	     //�������ת��  
	ADC1->CR2|=1<<20;      //ʹ�����ⲿ����(SWSTART)!!!	����ʹ��һ���¼�������
	ADC1->CR2&=~(1<<11);   //�Ҷ���	
	 
	ADC1->SQR1&=~(0XF<<20);
	ADC1->SQR1|=0<<20;     //1��ת���ڹ��������� Ҳ����ֻת����������1 	
				   
	//����ͨ��1�Ĳ���ʱ��
	ADC1->SMPR2&=~(7<<3);  //ͨ��1����ʱ�����	  
 	ADC1->SMPR2|=7<<3;     //ͨ��1  239.5����,��߲���ʱ�������߾�ȷ��
		 
	ADC1->SMPR1&=~(7<<18);  //���ͨ��16ԭ��������		 
	ADC1->SMPR1|=7<<18;     //ͨ��16  239.5����,��߲���ʱ�������߾�ȷ��	 	 
		 
		 
	ADC1->CR2|=1<<0;	   //����ADת����	 
	ADC1->CR2|=1<<3;       //ʹ�ܸ�λУ׼  

	while(ADC1->CR2&1<<3); //�ȴ�У׼���� 			 
       //��λ��������ò���Ӳ���������У׼�Ĵ�������ʼ�����λ��������� 		 
	ADC1->CR2|=1<<2;        //����ADУ׼	   
	while(ADC1->CR2&1<<2);  //�ȴ�У׼����
	//��λ����������Կ�ʼУ׼������У׼����ʱ��Ӳ�����  
}				  

//��ȡͨ��ch��ת��ֵ��ȡtimes��,Ȼ��ƽ�� 
//ch:ͨ�����
//times:��ȡ����
//����ֵ:ͨ��ch��times��ת�����ƽ��ֵ
//=======================================================	5
void Get_Adc_Average1(u16 times)
{			
 if(ADC_Ch_cnt==0)  //ҡ��X
 {
	  ADC1->SQR3&=0XFFFFFFF0;//ͨ��1 ��������1 ͨ��ch
	  ADC1->SQR3|=10;
	  ADC1->CR2|=1<<22;
	if(ADC1->SR&1<<1)
	{
	  ADC_cnt1++;
	  ADC_tempdata1+=ADC1->DR;	  
	}

	if(ADC_cnt1>=times)
	{
	 ADC_tempdata1/=times;
	 ADC_out1=ADC_tempdata1;
	 ADC_tempdata1=0;
	 ADC_cnt1=0;
	 ADC_Ch_cnt=3;
   if(ADC_out1<500)
   {
     adctemp1=0;
		 adctemp2=0; 
   }
	 if(ADC_out1>=1000)
	 {
		 adctemp1=(float)ADC_out1-1000.0;
		 adctemp2=adctemp1*0.2+adctemp1;

		 
   }
    if(adctemp2>4000.0)adctemp2=4000.0;
    ADCsim1=(u16)adctemp2;
 
 
		
	}
 }
//=======================================================6

   if((ADC_Ch_cnt>=2))	 //�жϼ������ڷ�Χ��  0.2139   0.2248
	 ADC_Ch_cnt=0; 

	//reference_V=reference_Vtemp=ADC_out5*1000*2/805;//���㵱ǰ��ѹ  	  3347
	 	 
	  

//================================================================	

//	  ADC2_A=ADC_out3*12930/10000;  //���ֵ��Ҳ���ֶ�	
//	  ADC2_B=ADC_out4*12930/10000;
		  

}

//========================================================











/*
//4.557  0.66

3000/4095=0.732

��λ�����ֵ������ֵ4095�Ĳ���Ե�λ�����ֵ��









*/




