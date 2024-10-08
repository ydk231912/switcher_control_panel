#include "sys.h"
#include "adc.h"
#include "led.h"
//#include "tim.h"


u32  ADC_tempdata1;	  		//ADC转换
u16  ADC_out1;
u16  ADC_cnt1;
u16  ADCsim1;    			//输出模拟值
u8 ADC_Ch_cnt;   			//通道转换计数
float adctemp1=0,adctemp2=0;

//===================================================================================
void  Adc_Init(void)		//173
{    

	//先初始化IO口  		//ADC12_IN5 6 7 8 9
	
 	RCC->APB2ENR|=1<<4;    	//使能PORTB口时钟 
	GPIOC->CRL&=0XFFFFFFF0;  
	 
	RCC->APB2ENR|=1<<9;    	//ADC1时钟使能	  
	RCC->APB2RSTR|=1<<9;   	//ADC1复位
	RCC->APB2RSTR&=~(1<<9);	//复位结束	    
	RCC->CFGR&=~(3<<14);   	//分频因子清零	

	RCC->CFGR|=2<<14;      	 
	ADC1->CR1&=0XF0FFFF;   	//工作模式清零
	ADC1->CR1|=0<<16;      	//独立工作模式  
	ADC1->CR1&=~(1<<8);    	//非扫描模式	
	  
	ADC1->CR2&=~(1<<1);    	//单次转换模式
	ADC1->CR2&=~(7<<17);	   
	ADC1->CR2|=7<<17;	   	//软件控制转换  
	ADC1->CR2|=1<<20;      	//使用外部触发(SWSTART)，必须使用一个事件来触发
	ADC1->CR2&=~(1<<11);   	//右对齐	
	 
	ADC1->SQR1&=~(0XF<<20);
	ADC1->SQR1|=0<<20;     	//1个转换在规则序列中 也就是只转换规则序列1 	
				   
	//设置通道1的采样时间
	ADC1->SMPR2&=~(7<<3);  	//通道1采样时间清空	  
 	ADC1->SMPR2|=7<<3;     	//通道1  239.5周期,提高采样时间可以提高精确度
		 
	ADC1->SMPR1&=~(7<<18);  //清除通道16原来的设置		 
	ADC1->SMPR1|=7<<18;     //通道16  239.5周期,提高采样时间可以提高精确度	 	 
		 
	ADC1->CR2|=1<<0;	   	//开启AD转换器	 
	ADC1->CR2|=1<<3;       	//使能复位校准  

	while(ADC1->CR2&1<<3); 	//等待校准结束 			 
	//该位由软件设置并由硬件清除。在校准寄存器被初始化后该位将被清除。 		 
	ADC1->CR2|=1<<2;        //开启AD校准	   
	while(ADC1->CR2&1<<2);  //等待校准结束
	//该位由软件设置以开始校准，并在校准结束时由硬件清除  
}				  

//=================================================================================== 5
//获取通道ch的转换值，取times次,然后平均 
//ch:通道编号
//times:获取次数
//返回值:通道ch的times次转换结果平均值
void Get_Adc_Average1(u16 times)
{			
	if(ADC_Ch_cnt==0)  				//摇杆X
	{
		ADC1->SQR3&=0XFFFFFFF0;		// 清除通道设置（规则序列1）
		ADC1->SQR3|=10;				// 设置通道10到规则序列1中
		ADC1->CR2|=1<<22;			// 启动ADC转换，通常是设置特定的位以启动转换
		if(ADC1->SR&1<<1)			// 检查是否有转换完成标志（EOC标志）
		{
			ADC_cnt1++;				// 记录有效的ADC读数次数
			ADC_tempdata1+=ADC1->DR;// 累加ADC数据寄存器的值	  
		}

		if(ADC_cnt1>=times)			// 判断是否已采集足够的数据
		{
			ADC_tempdata1/=times;	// 计算采样数据的平均值
			ADC_out1=ADC_tempdata1;	// 将平均值存储到ADC_out1
			ADC_tempdata1=0;		// 清空临时数据
			ADC_cnt1=0;				// 重置计数器
			ADC_Ch_cnt=3;			// 更新通道计数，表示处理完成
			if(ADC_out1<900)		// 根据ADC输出的值进行处理
			{
				adctemp1=0;
				adctemp2=0; 
			}
			if(ADC_out1>=900)
			{
				adctemp1=(float)ADC_out1-900.0;	// 计算与900的偏差
				adctemp2=adctemp1*0.55+adctemp1;// 计算加权平均
			}
			if(adctemp2>4000.0)adctemp2=4000.0;	// 限制值范围
			ADCsim1=(u16)adctemp2;				// 将最终的处理结果存储到ADCsim1
		}
	}
//=======================================================6

	if((ADC_Ch_cnt>=2))	 ADC_Ch_cnt=0;				//判断计数器在范围内  0.2139   0.2248
	  

//reference_V=reference_Vtemp=ADC_out5*1000*2/805;	//计算当前电压  	  3347
	 	 
	  

//================================================================	

//	  ADC2_A=ADC_out3*12930/10000;  				//零点值，也可手动	
//	  ADC2_B=ADC_out4*12930/10000;
		  

}




/*
//4.557  0.66

3000/4095=0.732

电位器最大值与满载值4095的差，除以电位器最大值，









*/




