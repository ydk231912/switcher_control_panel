#include "oledfont.h"
#include "delay.h"
#include "oled.h"
u8 CSse=0; //显示屏选择

//==========================================================
void OLED_Init(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOB|RCC_APB2Periph_GPIOC|RCC_APB2Periph_GPIOD|RCC_APB2Periph_GPIOE, ENABLE);	 //使能A端口时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);         //端口重映射
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);     //pin端口重置
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3|GPIO_Pin_5;	 
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//速度50MHz
 	GPIO_Init(GPIOB, &GPIO_InitStructure);	  

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//速度50MHz
 	GPIO_Init(GPIOE, &GPIO_InitStructure);	 
 	GPIO_SetBits(GPIOE,GPIO_Pin_15);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8|GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//速度50MHz
 	GPIO_Init(GPIOD, &GPIO_InitStructure);	 
		
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10|GPIO_Pin_11|GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//速度50MHz
 	GPIO_Init(GPIOC, &GPIO_InitStructure);	 
 	GPIO_SetBits(GPIOC,GPIO_Pin_15);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//速度50MHz
 	GPIO_Init(GPIOD, &GPIO_InitStructure);	
}

//=====================================================================================	
void initialize(void) 
{
	if(CSse==0){CS1=0;CS2=1;CS3=1;RES1=1;RES2=0;RES3=1;}
	if(CSse==1){CS1=1;CS2=0;CS3=1;RES1=1;RES2=1;RES3=1;} 
	if(CSse==2){CS1=1;CS2=1;CS3=0;RES1=1;RES2=1;RES3=1;}

	delay_ms(100);	

	OLED_WR_REG(0xae);//关闭显示
	OLED_WR_REG(0xfd);//命令锁
	OLED_WR_Byte(0x12); 
		
	if(USE_HORIZONTAL)
	{
		OLED_WR_REG(0xA0);  
		OLED_WR_Byte(0x24);	
		OLED_WR_Byte(0x10);	

		OLED_WR_REG(0xa2);//A2.Display Offset
		OLED_WR_Byte(0x80); //Offset:: 0~127				
	}
	else
	{
		OLED_WR_REG(0xA0);  
		OLED_WR_Byte(0x32);	
		OLED_WR_Byte(0x10);	

		OLED_WR_REG(0xa2);//A2.Display Offset
		OLED_WR_Byte(0x10);//Offset:: 0~127		
	}

	OLED_WR_REG(0xa1);//设置显示起始
	OLED_WR_Byte(0x00); 

//	OLED_WR_REG(0xa2);//设置显示偏移
//	OLED_WR_Byte(0x00); 

	OLED_WR_REG(0xa6);//正常显示  颜色反向
	OLED_WR_REG(0xad);//设置 IREF 
	OLED_WR_Byte(0x80); 
	 
	OLED_WR_REG(0xb1);//设置复位周期
	OLED_WR_Byte(0x74); 

	OLED_WR_REG(0xb3);//设置时钟分频
	OLED_WR_Byte(0xb1); 

	OLED_WR_REG(0xb6);//设置第二次预充
	OLED_WR_Byte(0x08); 

	OLED_WR_REG(0xb9);//设置默认线性灰度
	OLED_WR_REG(0xba);//设置预充电配置
	OLED_WR_Byte(0x02); 

	OLED_WR_REG(0xbb);//设置充电电压
	OLED_WR_Byte(0x07); 

	OLED_WR_REG(0xbe);//Set VCOMH 
	OLED_WR_Byte(0x07); 

	OLED_WR_REG(0xc1);//设置对比度
	OLED_WR_Byte(0x4f);//VCC=14V 

	OLED_WR_REG(0xca);//设置多路复用
	OLED_WR_Byte(0x7f); 
// 	CleanDDR(); 
	clear( );//清空内存
	OLED_WR_REG(0xaf);//开启显示
}

//=====================================================================================	
void clear(void)
{
	int i,j;
		
	OLED_WR_REG(0x15);//Set column address  设置列地址
	OLED_WR_REG(0x00);//Column Start Address设置起始地址
	OLED_WR_REG(0x3f);//Column End Address  结束地址
	OLED_WR_REG(0x75);//Set row address   	行地址
	OLED_WR_REG(0x00);//Row Start Address   行起始地址
	OLED_WR_REG(0x7f);//Row End Address    	行结束地址
	for(i=0;i<128;i++)
	{
		for(j=0;j<64;j++)
		{
			OLED_WR_Byte(0x00);
		}
	}
}

//=====================================================================================	
//******************************************************************************
//    函数说明：OLED写入一个指令
//    入口数据：dat 数据
//    返回值：  无
//******************************************************************************
void OLED_WR_Bus(u8 dat)
{
	u8 i;

	if(CSse==0){CS1=0;CS2=1;CS3=1;}
	if(CSse==1){CS1=1;CS2=0;CS3=1;}
	if(CSse==2){CS1=1;CS2=1;CS3=0;}
	for(i=0;i<8;i++)
	{			  
		SSCL=0;
		if(dat&0x80)SSDA=1;
		else SSDA=0;
		SSCL=1;
		dat<<=1;   
	}				 		  
	CS1=1;CS2=1;CS3=1;
}

//=====================================================================================	
//******************************************************************************
//    函数说明：OLED写入一个指令
//    入口数据：reg 指令
//    返回值：  无
//******************************************************************************
void OLED_WR_REG(u8 reg)
{	  
	if(CSse==0){DC1=0;DC2=1;DC3=1;}
	if(CSse==1){DC1=1;DC2=0;DC3=1;}
	if(CSse==2){DC1=1;DC2=1;DC3=0;}
	OLED_WR_Bus(reg);
	DC1=1;DC2=1;DC3=1;	
}

//=====================================================================================	
//******************************************************************************
//    函数说明：OLED写入一个数据
//    入口数据：dat 数据
//    返回值：  无
//******************************************************************************
void OLED_WR_Byte(u8 dat)
{	  
	OLED_WR_Bus(dat);
}

//=====================================================================================	
//******************************************************************************
//    函数说明：OLED显示列的起始终止地址
//    入口数据：a  列的起始地址
//              b  列的终止地址
//    返回值：  无
//******************************************************************************
void Column_Address(u8 a,u8 b)
{
	OLED_WR_REG(0x15);       // 设置列地址
	OLED_WR_Byte(a+0x08);
	OLED_WR_Byte(b+0x08);
}

//=====================================================================================	
//******************************************************************************
//    函数说明：OLED显示行的起始终止地址
//    入口数据：a  行的起始地址
//              b  行的终止地址
//    返回值：  无
//******************************************************************************
void Row_Address(u8 a,u8 b)
{
	OLED_WR_REG(0x75);    // 行列地址
	OLED_WR_Byte(a);
	OLED_WR_Byte(b);
	OLED_WR_REG(0x5C);    //写RAM命令
}

//=====================================================================================	
void OLED_Fill(u16 xstr,u16 ystr,u16 xend,u16 yend,u8 color)
{
	u8 x,y;
	xstr/=4; // 控制4列 
	xend/=4;
	Column_Address(xstr,xend-1);
	Row_Address(ystr,yend-1);
	for(x=xstr;x<xend;x++)
	{
		for(y=ystr;y<yend;y++)
		{
			OLED_WR_Byte(color); //控制4列 同时支持16阶灰度 所以需要2个字节
			OLED_WR_Byte(color);
		}
	}
}

//=====================================================================================	
//******************************************************************************
//    函数说明：OLED显示字符函数 
//    此函数适用范围：字符宽度是4的倍数  字符高度是宽度的2倍
//    入口数据：x,y   起始坐标
//              chr   要写入的字符
//              sizey 字符高度 
//              mode  0:正常显示；1：反色显示
//    返回值：  无
//******************************************************************************

void OLED_ShowChar(u8 x,u8 y,u8 chr,u8 sizey,u8 mode)
{
	u8 sizex,c,temp,t=2,DATA1=0,DATA2=0,m;
	u16 i,k,size2;
	x/=4;
	sizex=sizey/4/2; 							//除4是因为一个列地址线控制4列 除2是字宽:字高 1:2
	size2=(sizey/16+((sizey%16)?1:0))*sizey; 	//计算一个字符所占字节数
	c=chr-' '; 									//ASCII码值对应偏移后的值
	Column_Address(x,x+sizex-1);				//设置列地址
	Row_Address(y,y+sizey-1);					//设置行地址
	for(i=0;i<size2;i++)
	{
		if(sizey==16)
		{
			temp=ascii_1608[c][i]; 				//获取取模数据 
		}
		else if(sizey==24)
		{
			temp=ascii_2412[c][i];				//12x24 ASCII码
		}
		else if(sizey==32)
		{
			temp=ascii_3216[c][i];				//16x32 ASCII码
		}
		if(sizey%16)
		{
			m=sizey/16+1;
			if(i%m) t=1;
			else t=2;
		}
		for(k=0;k<t;k++)
		{
			if(temp&(0x01<<(k*4)))
			{
				DATA1=0x0F;
			}
			if(temp&(0x01<<(k*4+1)))
			{
				DATA1|=0xF0;
			}
			if(temp&(0x01<<(k*4+2)))
			{
				DATA2=0x0F;
			}
			if(temp&(0x01<<(k*4+3)))
			{
				DATA2|=0xF0;
			}
			if(mode)
			{
				OLED_WR_Byte(~DATA2);
				OLED_WR_Byte(~DATA1);		
			}
			else
			{	
				OLED_WR_Byte(DATA2);
				OLED_WR_Byte(DATA1);
			}
			DATA1=0;
			DATA2=0;
		}
	}
}

//=====================================================================================	
//******************************************************************************
//    函数说明：OLED显示字符串
//    入口数据：x,y  起始坐标
//              *dp   要写入的字符
//              sizey 字符高度 
//              mode  0:正常显示；1：反色显示
//    返回值：  无
//******************************************************************************
void OLED_ShowString(u8 x,u8 y,u8 *dp,u8 sizey,u8 mode)
{
	while(*dp!='\0')
	{
	  OLED_ShowChar(x,y,*dp,sizey,mode);
		dp++;
		x+=sizey/2;
	}
}

//=====================================================================================	
//******************************************************************************
//    函数说明：m^n
//    入口数据：m:底数 n:指数
//    返回值：  result
//******************************************************************************
u32 oled_pow(u8 m,u8 n)
{
	u32 result=1;
	while(n--)result*=m;    
	return result;
}

//=====================================================================================	
//******************************************************************************
//    函数说明：OLED显示变量
//    入口数据：x,y :起点坐标	 
//              num :要显示的变量
//              len :数字的位数
//              sizey 字符高度 
//              mode  0:正常显示；1：反色显示
//    返回值：  无
//******************************************************************************
void OLED_ShowNum(u8 x,u8 y,u32 num,u8 len,u8 sizey,u8 mode)
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

//=====================================================================================
//******************************************************************************
//    函数说明：OLED屏初始化
//    入口数据：无
//    返回值：  无
//******************************************************************************
/*
		OLED_CS_Clr(); 
		OLED_RES_Set();	
		delay_ms(10);	

		OLED_WR_REG(0xae);//Set Display OFF //关闭显示
		OLED_WR_REG(0xfd);//Set Command Lock //命令锁
		OLED_WR_Byte(0x12); 
		
		OLED_WR_REG(0xa0);//Set Re-map and Dual COM Line mode 设置重映射
		OLED_WR_Byte(0x14); 
		OLED_WR_Byte(0x10);
		
		
		
		OLED_WR_REG(0xa1);//Set Display Start Line 设置显示起始
		OLED_WR_Byte(0x00); 
		
		OLED_WR_REG(0xa2);//Set Display Offset 设置显示偏移
		OLED_WR_Byte(0x00); 
		
		OLED_WR_REG(0xa6);//Normal Display   正常显示  颜色反向
		OLED_WR_REG(0xad);//Set IREF 
		OLED_WR_Byte(0x80); 
		 
		OLED_WR_REG(0xb1);//Set Reset (Phase 1)/Pre-charge (Phase 2)period 设置重置
		OLED_WR_Byte(0x74); 
		
		OLED_WR_REG(0xb3);//Set Front Clock Divider/Oscillator Frequency 设置时钟分频
		OLED_WR_Byte(0xb1); 
		
		OLED_WR_REG(0xb6);//Set Second Pre-charge Period 设置第二次预充
		OLED_WR_Byte(0x08); 
		
		OLED_WR_REG(0xb9);//Select Default Linear Gray Scale table 设置默认线性灰度
		OLED_WR_REG(0xba);//Set Pre-charge voltage configuration 设置充电配置
		OLED_WR_Byte(0x02); 
		
		OLED_WR_REG(0xbb);//Set Pre-charge voltage 设置充电电压
		OLED_WR_Byte(0x07); 
		
		OLED_WR_REG(0xbe);//Set VCOMH 
		OLED_WR_Byte(0x07); 
		
		OLED_WR_REG(0xc1);//Set Contrast Current 设置对比度
		OLED_WR_Byte(0x4f);//VCC=14V 
		
		OLED_WR_REG(0xca);//Set MUX Ratio 设置多路复用
		OLED_WR_Byte(0x7f); 
// 		CleanDDR(); 
		clear( );// clear the whole DDRAM. 清空内存
		OLED_WR_REG(0xaf);//Set Display ON
*/
		
		
		
		
		


