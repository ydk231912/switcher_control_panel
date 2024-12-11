#include "led.h"
#include "delay.h"

void LED_Init(void)
{
 
 GPIO_InitTypeDef  GPIO_InitStructure;
 	
 RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);	 //使能PB,PE端口时钟
 GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);     //pin端口重置
 RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14;	 //485EN PA0 PA8
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 //推挽输出
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 //IO口速度为50MHz
  GPIO_Init(GPIOD, &GPIO_InitStructure);					       //根据设定参数初始化GPIOB.5
  GPIO_SetBits(GPIOD,GPIO_Pin_14);		
	
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;	 //485EN PA0 PA8
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 //推挽输出
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 //IO口速度为50MHz
  GPIO_Init(GPIOD, &GPIO_InitStructure);					       //根据设定参数初始化GPIOB.5
  GPIO_SetBits(GPIOD,GPIO_Pin_5);
	


}
 


void KEY_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB|RCC_APB2Periph_GPIOA|RCC_APB2Periph_AFIO, ENABLE);	 //使能PB,PE端口时钟
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC|RCC_APB2Periph_GPIOD|RCC_APB2Periph_GPIOE, ENABLE);

	GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_4|GPIO_Pin_5;//
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; //设置成上拉输入
 	GPIO_Init(GPIOC, &GPIO_InitStructure);//
	
	
	GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_1|GPIO_Pin_0|GPIO_Pin_7|GPIO_Pin_8|GPIO_Pin_9|GPIO_Pin_6;//
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; //设置成上拉输入
 	GPIO_Init(GPIOB, &GPIO_InitStructure);//初始化GPIOE2,3,4
	
	
	
  GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_9|GPIO_Pin_7|GPIO_Pin_10|GPIO_Pin_8|GPIO_Pin_11|GPIO_Pin_14;//
	GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_12|GPIO_Pin_13|GPIO_Pin_15;//
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; //设置成上拉输入
 	GPIO_Init(GPIOE, &GPIO_InitStructure);//
	
	
	GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_6|GPIO_Pin_7|GPIO_Pin_11|GPIO_Pin_12|GPIO_Pin_9|GPIO_Pin_8;//
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; //设置成上拉输入
 	GPIO_Init(GPIOD, &GPIO_InitStructure);//
	

	
}






