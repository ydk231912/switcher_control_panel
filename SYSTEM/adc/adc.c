#include "sys.h"
#include "adc.h"
#include "led.h"
//#include "tim.h"

u32 ADC_tempdata1;	  //ADC转换
u16 ADC_out1;
u16  ADC_cnt1;

u32 ADC_tempdata2;	  //ADC转换
u16 ADC_out2;
u16 ADC_cnt2;

u32 ADC_tempdata3;	  //ADC转换
u16 ADC_out3;
u16 ADC_cnt3;

u32 ADC_tempdata4;	  //ADC转换
u16 ADC_out4;
u16 ADC_cnt4;

u32 ADC_tempdata4;	  //ADC转换
u16 ADC_out4;
u16 ADC_cnt4;

u32 ADC_tempdata5;	  //ADC转换
u16 ADC_out5;
u16 ADC_cnt5;

u16 reference_V;     //当前电压
u32 reference_Vtemp;

u8 ADC_Ch_cnt;   //通道转换计数

s16 ADC1_X,ADC1_Y,ADC2_A,ADC2_B;

s16 ADCtemp[10];   //电位器校零值

//=====================================================================================	
void  Adc_Init(void)	   //173
{    

	//先初始化IO口  /ADC12_IN5 6 7 8 9
	
	RCC->APB2ENR|=1<<2;    //使能PORTA口时钟 
	GPIOA->CRL&=0X000FFFFF;//PA1 anolog输入  PA5 6 7 
	
 	RCC->APB2ENR|=1<<3;    //使能PORTB口时钟 
	GPIOB->CRL&=0XFFFFFF00;//PB1 anolog输入  PB0 1
	 
	RCC->APB2ENR|=1<<9;    //ADC1时钟使能	  
	RCC->APB2RSTR|=1<<9;   //ADC1复位
	RCC->APB2RSTR&=~(1<<9);//复位结束	    
	RCC->CFGR&=~(3<<14);   //分频因子清零	

	RCC->CFGR|=2<<14;      //选择 PLL 作为系统时钟源
	ADC1->CR1&=0XF0FFFF;   //工作模式清零
	ADC1->CR1|=0<<16;      //独立工作模式  
	ADC1->CR1&=~(1<<8);    //非扫描模式	
	  
	ADC1->CR2&=~(1<<1);    //单次转换模式
	ADC1->CR2&=~(7<<17);	   
	ADC1->CR2|=7<<17;	   //软件控制转换  
	ADC1->CR2|=1<<20;      //使用用外部触发(SWSTART)!!!	必须使用一个事件来触发
	ADC1->CR2&=~(1<<11);   //右对齐	
	 
	ADC1->SQR1&=~(0XF<<20);
	ADC1->SQR1|=0<<20;     //1个转换在规则序列中 也就是只转换规则序列1 	
				   
	//设置通道1的采样时间
	ADC1->SMPR2&=~(7<<3);  //通道1采样时间清空	  
 	ADC1->SMPR2|=7<<3;     //通道1  239.5周期,提高采样时间可以提高精确度
		 
	ADC1->SMPR1&=~(7<<18);  //清除通道16原来的设置		 
	ADC1->SMPR1|=7<<18;     //通道16  239.5周期,提高采样时间可以提高精确度	 	 
		 
		 
	ADC1->CR2|=1<<0;	   //开启AD转换器	 
	ADC1->CR2|=1<<3;       //使能复位校准  

	while(ADC1->CR2&1<<3); //等待校准结束 			 
       //该位由软件设置并由硬件清除。在校准寄存器被初始化后该位将被清除。 		 
	ADC1->CR2|=1<<2;        //开启AD校准	   
	while(ADC1->CR2&1<<2);  //等待校准结束
	//该位由软件设置以开始校准，并在校准结束时由硬件清除  
}				  

//=====================================================================================	5
//获取通道ch的转换值，取times次,然后平均 
//ch:通道编号
//times:获取次数
//返回值:通道ch的times次转换结果平均值
void Get_Adc_Average1(u16 times)
{			
 if(ADC_Ch_cnt==0)  //摇杆X
 {
	  ADC1->SQR3&=0XFFFFFFF0;//通道1 规则序列1 通道ch
	  ADC1->SQR3|=5;
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
	 ADC_Ch_cnt=1;	
	}
 }
//=======================================================6

  if(ADC_Ch_cnt==1)  //通道2摇杆Y
	{
	 ADC1->SQR3&=0XFFFFFFF0;//规则序列1 通道ch
	 ADC1->SQR3|=6;
	 ADC1->CR2|=1<<22;

	 if(ADC1->SR&1<<1)
	 {
	  ADC_cnt2++;
		ADC_tempdata2+=ADC1->DR;
		
	 }

	if(ADC_cnt2>=times)
	{
	 ADC_tempdata2/=times;
	 ADC_out2=ADC_tempdata2;
	 ADC_tempdata2=0;
	 ADC_cnt2=0;
	 ADC_Ch_cnt=2;
	}
   } 
//=======================================================	7
	if(ADC_Ch_cnt==2)  //
	{
	 ADC1->CR2|=1<<23;      //电位器A
	 ADC1->SQR3&=0XFFFFFFF0;//规则序列1 通道ch
	 ADC1->SQR3|=7;
	 ADC1->CR2|=1<<22;

	 if(ADC1->SR&1<<1)
	 {
	  ADC_cnt3++;
		ADC_tempdata3+=ADC1->DR;		
	 }

	if(ADC_cnt3>=times)
	{
	 ADC_tempdata3/=times;
	 ADC_out3=ADC_tempdata3;
	 ADC_tempdata3=0;
	 ADC_cnt3=0;
	 ADC_Ch_cnt=3;	
	}
	}
//=======================================================	8
	
	if(ADC_Ch_cnt==3)  //
	{
	 ADC1->CR2|=1<<23;      //电位器B
	 ADC1->SQR3&=0XFFFFFFF0;//规则序列1 通道ch
	 ADC1->SQR3|=8;
	 ADC1->CR2|=1<<22;

	 if(ADC1->SR&1<<1)
	 {
	  ADC_cnt4++;
		ADC_tempdata4+=ADC1->DR;
		
	 }

	if(ADC_cnt4>=times)
	{
	 ADC_tempdata4/=times;
	 ADC_out4=ADC_tempdata4;
	 ADC_tempdata4=0;
	 ADC_cnt4=0;
	 ADC_Ch_cnt=4;
	}
	}
	
//=======================================================9		
	
if(ADC_Ch_cnt==4)  //
	{
	 ADC1->CR2|=1<<23;      //电压检测
	 ADC1->SQR3&=0XFFFFFFF0;//规则序列1 通道ch
	 ADC1->SQR3|=9;
	 ADC1->CR2|=1<<22;

	 if(ADC1->SR&1<<1)
	 {
	  ADC_cnt5++;
		ADC_tempdata5+=ADC1->DR;		
	 }

	if(ADC_cnt5>=times)
	{
	 ADC_tempdata5/=times;
	 ADC_out5=ADC_tempdata5;
	 ADC_tempdata5=0;
	 ADC_cnt5=0;
	 ADC_Ch_cnt=5;
	}
	}	
	
   if((ADC_Ch_cnt>=5))	 //判断计数器在范围内  0.2139   0.2248
	 ADC_Ch_cnt=0; 

	 //reference_V=reference_Vtemp=ADC_out5*1000*2/805;//计算当前电压  	  3347
	 	 
	 reference_V=ADC_out5;   //采集外部电压
	  
//===============摇杆================================================	 
	/* 
	 ADC_Time++;   //上电延时校正电位器  ADCtemp[0]=ADC_out1*12378/10000; 
	 
	 if((ADC_Time>800)&&(ADC_Time<900))
	 {
		 ADCtemp[0]=ADC_out1*14378/10000;  //零点
	   ADCtemp[1]=ADC_out2*14378/10000;		 
   }	
	 
	 else if(ADC_Time>900)
	 {
		  ADC1_X=ADC_out1*14378/10000-ADCtemp[0];	 
	    ADC1_Y=ADC_out2*14378/10000-ADCtemp[1];
   }
	 else ;
	 	 
	 if(ADC_Time>900)	ADC_Time=10000;

	 
	   if(ADC1_X>2047)ADC1_X=2048;            //大于输出值限制
	   if(ADC1_X<(-2047))ADC1_X=-2048; 
	 
	   if((ADC1_X>0)&&(ADC1_X<90))ADC1_X=0;   //小于输出值限制
	   if((ADC1_X<0)&&(ADC1_X>-90))ADC1_X=0;
 
	   if(ADC1_Y>2047)ADC1_Y=2048;            //大于输出值限制
	   if(ADC1_Y<(-2047))ADC1_Y=-2048; 
	 
	   if((ADC1_Y>0)&&(ADC1_Y<90))ADC1_Y=0;   //小于输出值限制
	   if((ADC1_Y<0)&&(ADC1_Y>-90))ADC1_Y=0;
	 	*/
//================================================================	

	  ADC2_A=ADC_out3*12930/10000;  //零点值，也可手动	
	  ADC2_B=ADC_out4*12930/10000;
		  
	  if(ADC2_A>4095)ADC2_A=4095;   //最大最小数据限制
	  if(ADC2_A<60)ADC2_A=0;
	  if(ADC2_B>4095)ADC2_B=4095;
	  if(ADC2_B<60)ADC2_B=0;
	
//
}

//========================================================

void ADC_zero_calibration(void)  //编码器零点
{
	;
	
}












/*
//4.557  0.66

3000/4095=0.732

电位器最大值与满载值4095的差，除以电位器最大值，









*/




