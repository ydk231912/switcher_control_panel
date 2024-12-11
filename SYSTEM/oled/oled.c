#include "oled.h"
#include "stdlib.h"
#include "oledfont.h"  	 
#include "delay.h"
#include "spi.h"
#include "dma.h"

//******************************************************************************
//    函数说明：OLED写入一个指令
//    入口数据：dat 数据
//    返回值：  无
//******************************************************************************
void OLED_WR_Bus(u8 dat)
{
	OLED_CS_Clr();
	SPI3_ReadWriteByte(dat);
	OLED_CS_Set();
}

//******************************************************************************
//    函数说明：OLED写入一个指令
//    入口数据：reg 指令
//    返回值：  无
//******************************************************************************
void OLED_WR_REG(u8 reg)
{	  
	OLED_DC_Clr();		  
  OLED_WR_Bus(reg);
  OLED_DC_Set();	
}

//******************************************************************************
//    函数说明：OLED写入一个数据
//    入口数据：dat 数据
//    返回值：  无
//******************************************************************************
void OLED_WR_Byte(u8 dat)
{	  
  OLED_WR_Bus(dat);
}

//******************************************************************************
//    函数说明：OLED显示列的起始终止地址
//    入口数据：a  列的起始地址
//              b  列的终止地址
//    返回值：  无
//******************************************************************************
void OLED_AddressSet(u8 x,u8 y) 
{
	OLED_WR_REG(0xB0);
	OLED_WR_REG(y);
	OLED_WR_REG(((x&0xf0)>>4)|0x10);
	OLED_WR_REG((x&0x0f));
}

//******************************************************************************
//    函数说明：OLED清屏显示
//    入口数据：无
//    返回值：  无
//******************************************************************************
void OLED_Clear(void)
{
	u16 j,i;
	OLED_AddressSet(0,0);
	OLED_CS_Clr(); //片选

	for(i=0;i<64;i++)
	{
		OLED_AddressSet(0,i);
		for(j=0;j<128;j++)
		{
			OLED_WR_Byte(0x00);
		}
	}
	OLED_CS_Set();
}

//******************************************************************************
//    函数说明：OLED清屏显示
//    入口数据：x1,y1 起点坐标
//              x2,y2 结束坐标
//              color 填充的颜色值
//    返回值：  无
//******************************************************************************
void OLED_Fill(u16 x1,u8 y1,u16 x2,u8 y2,u8 color)
{
	u16 j,i;
	x1/=2;
	x2/=2;
	for(i=y1;i<y2;i++)
	{
		OLED_AddressSet(x1,i);
		for(j=x1;j<x2;j++)
		{
			OLED_WR_Byte(color);
		}
	}
}

//******************************************************************************
//    函数说明：OLED显示字符函数 
//    此函数适用范围：字符宽度是2的倍数  字符高度是宽度的2倍
//    入口数据：x,y   起始坐标
//              chr   要写入的字符
//              sizey 字符高度 
//    返回值：  无
//******************************************************************************
void OLED_ShowChar(u8 x,u8 y,u8 chr,u8 sizey,u8 mode)
{
	u8 i,j,k,t=4,temp,DATA=0;
	
	x/=2;
	k=sizey/64;	
//	for(i=0;i<254;i++)
	for(i=0;i<25;i++)
	{
		if(mode==0)
		{
		 temp=0xf8;//调用24x48字符
		}
		else
		{
		 temp=0;//调用24x48字符	
    }
		
		if(sizey%16)
		{
			k=sizey/16+1;
			if(i%k) t=2;
			else t=4;
		}
		if(i%k==0)
		{
			OLED_AddressSet(x,y);
			y++;
		}
		for(j=0;j<t;j++)
		{
			if(temp&(0x01<<(j*2+0)))
			{
				DATA=0xf0;
			}
			if(temp&(0x01<<(j*2+1)))
			{
				DATA|=0x0f;
			}
		//	if(mode)  //模式切换
			if(0)
			{
				OLED_WR_Byte(~DATA);
			}else
			{
				OLED_WR_Byte(DATA);
			}
			DATA=0;
		}
	}
}

//******************************************************************************
//    函数说明：OLED显示字符串
//    入口数据：x,y  起始坐标
//              *dp   要写入的字符
//              sizey 字符高度 
//              mode  0:正常显示；1：反色显示
//    返回值：  无
//******************************************************************************
//OLED_ShowString(0,8,oleddata,48,0);
void OLED_ShowString(u8 x,u8 y,u8 dp,u8 sizey,u8 mode)
{
	u8 i=0;
	for(i=0;i<25;i++)
	{
		if(i<dp)OLED_ShowChar(x,y,dp,sizey,1);
 		else OLED_ShowChar(x,y,dp,sizey,0);
		x+=sizey/6; //X轴间距 
  }
}

//******************************************************************************
//    函数说明：m^n
//    入口数据：m:底数 n:指数
//    返回值：  result
//******************************************************************************
u32 oled_pow(u16 m,u16 n)
{
	u32 result=1;
	while(n--)result*=m;    
	return result;
}


//******************************************************************************
//    函数说明：OLED显示变量
//    入口数据：x,y :起点坐标	 
//              num :要显示的变量
//              len :数字的位数
//              sizey 字符高度 
//              mode  0:正常显示；1：反色显示
//    返回值：  无
//******************************************************************************
void OLED_ShowNum(u8 x,u8 y,u32 num,u16 len,u8 sizey,u8 mode)
{         	
	u8 t,temp;
	u8 enshow=0;
	for(t=0;t<len;t++)
	{
		temp=(num/oled_pow(10,len-t-1))%10;
		if(enshow==0&&t<(len-1))
		{
			if(temp==0)
			{
				OLED_ShowChar(x+(sizey/2)*t,y,' ',sizey,mode);
				continue;
			}else enshow=1; 
		 	 
		}
	 	OLED_ShowChar(x+(sizey/2)*t,y,temp+'0',sizey,mode); 
	}
}


//******************************************************************************


void OLED_Init(void)
{
	 GPIO_InitTypeDef  GPIO_InitStructure;
   RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO|RCC_APB2Periph_GPIOD, ENABLE);

 //  GPIO_PinRemapConfig(GPIO_Remap_PD01, ENABLE);
//	 RCC_HSEConfig(RCC_HSE_OFF);  //关闭外时钟
	
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1|GPIO_Pin_2|GPIO_Pin_3;	 
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//速度50MHz
 	GPIO_Init(GPIOD, &GPIO_InitStructure);
	GPIO_SetBits(GPIOD,GPIO_Pin_1|GPIO_Pin_2|GPIO_Pin_3);
	
	
	
//	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO|RCC_APB2Periph_GPIOB, ENABLE);
//	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);     //pin???ú????

//	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_5;     //PD3,PD6í?íìê?3?
//	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;           //í?íìê?3?
//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;      //?ù?è50MHz
//	GPIO_Init(GPIOB, &GPIO_InitStructure);                   //3?ê??ˉGPIOD3,6
//	GPIO_SetBits(GPIOB,GPIO_Pin_3|GPIO_Pin_5);
	
	
	
	
	
	OLED_RES_Clr();//复位
	delay_ms(200);
	OLED_RES_Set();
	
	OLED_WR_REG(0xAE);//Set display off
	OLED_WR_REG(0xB0); //Row address Mode Setting
	OLED_WR_REG(0x00);
	OLED_WR_REG(0x10); //Set Higher Column Address of display RAM
	OLED_WR_REG(0x00); //Set Lower Column Address of display RAM
	OLED_WR_REG(0xD5); //Set Display Clock Divide Ratio/Oscillator Frequency
	OLED_WR_REG(0x50); //50 125hz
	OLED_WR_REG(0xD9); //Set Discharge/Precharge Period
	OLED_WR_REG(0x22);
	OLED_WR_REG(0x40); //Set Display Start Line
	OLED_WR_REG(0x81); //The Contrast Control Mode Set
	OLED_WR_REG(0x1F); //对比度设置
	if(USE_HORIZONTAL)
	{
		OLED_WR_REG(0xA1); //Set Segment Re-map
		OLED_WR_REG(0xC8); //Set Common Output Scan Direction
	  OLED_WR_REG(0xD3); //Set Display Offset
	  OLED_WR_REG(0x20);
	}else
	{
		OLED_WR_REG(0xA0); //Set Segment Re-map
		OLED_WR_REG(0xC0); //Set Common Output Scan Direction
	  OLED_WR_REG(0xD3); //Set Display Offset
	  OLED_WR_REG(0x00);
	}
	OLED_WR_REG(0xA4); //Set Entire Display OFF/ON
	OLED_WR_REG(0xA6); //Set Normal/Reverse Display
	OLED_WR_REG(0xA8); //Set Multiplex Ration
	OLED_WR_REG(0x3F);
	OLED_WR_REG(0xAD); //DC-DC Setting
	OLED_WR_REG(0x80); //DC-DC is disable
	OLED_WR_REG(0xDB); //Set VCOM Deselect Level
	OLED_WR_REG(0x30);
	OLED_WR_REG(0xDC); //Set VSEGM Level
	OLED_WR_REG(0x30);
	OLED_WR_REG(0x33); //Set Discharge VSL Level 1.8V
	OLED_Clear();
	OLED_WR_REG(0xAF); //Set Display On
}

