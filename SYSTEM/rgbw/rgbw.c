#include "rgbw.h"
#include "spi.h"
#include "dma.h"
#include "delay.h"
//将灯条显存SPI数据缓存
u8 gWs2812bDat_SPI[WS2812B_AMOUNT * 32] = {0};	//灯珠要发送数据数组
//灯条显存
u8 rgbw2814[WS2812B_AMOUNT]={0};  				//灯控制数组

//===================================================================================
//颜色 r红 g 绿 b 蓝 w 白
void WS2812b_Set(u16 Ws2b812b_NUM, u8 rgbw,u8 r, u8 g, u8 b,u8 w ,u8 temp)
{
	u8 i;

	//根据灯珠颜色调整顺序
	u8 *pR = &gWs2812bDat_SPI[(Ws2b812b_NUM) * 32 + 8];  //灯珠颜色赋值
	u8 *pW = &gWs2812bDat_SPI[(Ws2b812b_NUM) * 32];
	u8 *pG = &gWs2812bDat_SPI[(Ws2b812b_NUM) * 32 + 16];
	u8 *pB = &gWs2812bDat_SPI[(Ws2b812b_NUM) * 32 + 24];
	
	//判断灯珠每个颜色字节每位状态，每个状态位赋值给一个字节发出，组成一个位，每个灯珠颜色有64个位
	//组成，每8个位组成灯珠颜色字节的一个位，需要控制SPI通讯速率
	for(i = 0; i < 8; i++) 
	{
		if((rgbw==1)&&(i<w)) *pW = CODE_1;    //白 判断灯珠位开关，赋值给映射的一个字节
          
		else *pW = CODE_0; 
			
		if((rgbw==2)&&(i<r)) *pR = CODE_1;    //红 判断灯珠位开关，赋值给映射的一个字节
          
		else *pR = CODE_0; 
		
		
		if((rgbw==3)&&(i<g)) *pG = CODE_1;    // 判断灯珠位开关，赋值给映射的一个字节
          
		else *pG = CODE_0; 
		
		
		if((rgbw==4)&&(i<b)) *pB = CODE_1;    // 判断灯珠位开关，赋值给映射的一个字节
          
		else *pB = CODE_0; 
				
		pR++;      //指针增加，增加每字节对应一个位。
		pG++;
		pB++;
		pW++;
	}
}

//===================================================================================
void WS2812B_Task(void)
{
	u8 dat = 0;
	u8 iLED;
	u8 rgbwtemp;
	u16 len;
	len=sizeof(gWs2812bDat_SPI);
	//解析WS2814数据   //WS2812B_AMOUNT   8
	
	for( iLED = 0; iLED < WS2812B_AMOUNT; iLED++) //
	{
		rgbwtemp=rgbw2814[iLED]&0x0f;
		WS2812b_Set(iLED,rgbwtemp ,1,1,1,1,0);   //把灯珠颜色状态写入到gWs2812bDat_SPI发送数组
	}
	
	SPI2_WriteByte(gWs2812bDat_SPI,len);
	SPI2_WriteByte(&dat,2);
	
//	DMA_SPI2_Tx(gWs2812bDat_SPI,sizeof(gWs2812bDat_SPI));
//	DMA_SPI2_Tx(&dat,1)
}

//===================================================================================
/*
//以下程序为可以点亮单个RGBW程序，
u8 gWs2812bDat_SPI[WS2812B_AMOUNT * 32] = {0};	//灯珠要发送数据数组
//灯条显存
tWs2812bCache_TypeDef gWs2812bDat[WS2812B_AMOUNT] = {
//R    G      B    W
0X00, 0X00, 0X00,	0X0F,//0
0X00, 0X00, 0X00,	0X0F,//1
0X00, 0X00, 0X00,	0X0F,//2
0X00, 0X00, 0X00,	0X0F,//3
0X00, 0X00, 0X00,	0X0F,//4
0X00, 0X00, 0X00,	0X0F,//5
0X00, 0X00, 0X00,	0X0F,//6
0X00, 0X00, 0X00,	0X0F,//7
};

//===================================================================================
void WS2812b_Set(u16 Ws2b812b_NUM, u8 r,u8 g,u8 b,u8 w)
{
	u8 i;
	//根据灯珠颜色调整顺序
	uint8_t *pR = &gWs2812bDat_SPI[(Ws2b812b_NUM) * 32 + 8];  //灯珠颜色赋值
	uint8_t *pG = &gWs2812bDat_SPI[(Ws2b812b_NUM) * 32];
	uint8_t *pB = &gWs2812bDat_SPI[(Ws2b812b_NUM) * 32 + 16];
	uint8_t *pW = &gWs2812bDat_SPI[(Ws2b812b_NUM) * 32 + 24];
	

	

	
	//判断灯珠每个颜色字节每位状态，每个状态位赋值给一个字节发出，组成一个位，每个灯珠颜色有64个位
	//组成，每8个位组成灯珠颜色字节的一个位，需要控制SPI通讯速率
	for(i = 0; i < 8; i++) 
	{
		if(g & 0x80)     // 判断灯珠位开关，赋值给映射的一个字节
			{
			*pG = CODE_1;   //0xF8  1111 1000
		  }           
		else 
			{           
			*pG = CODE_0;   //0xE0  1110 0000
		  }           
		if(r & 0x80) 
			{           
			*pR = CODE_1;
		  }           
		else 
			{           
			*pR = CODE_0;
		  }           
		if(b & 0x80) 
			{           
			*pB = CODE_1;
		  }           
		else 
			{           
			*pB = CODE_0;
		  }
			
		if(w & 0x80) 
			{           
			*pW = CODE_1;
		  }    	
			else 
			{           
			*pW = CODE_0;
		  }
			
		  r <<= 1;  //移位判断每个颜色的位状态
		  g <<= 1;
	  	b <<= 1;
			w <<= 1;
		  pR++;      //指针增加，增加每字节对应一个位。
		  pG++;
		  pB++;
			pW++;
	}
}

//===================================================================================
void WS2812B_Task(void)
{
	u8 dat = 0;
	u8 iLED;
	//解析WS2814数据   //WS2812B_AMOUNT   8
	for( iLED = 0; iLED < WS2812B_AMOUNT; iLED++) //
	{
		//把灯珠颜色状态写入到gWs2812bDat_SPI发送数组
		WS2812b_Set(iLED, gWs2812bDat[iLED].R, gWs2812bDat[iLED].G, gWs2812bDat[iLED].B,gWs2812bDat[iLED].W);
	}
	//总线输出数据
	//HAL_SPI_Transmit(&hspi1, gWs2812bDat_SPI, sizeof(gWs2812bDat_SPI), 100);
	//使总线输出低电平
//	HAL_SPI_Transmit(&hspi1, &dat, 1, 100);
	//帧信号，一个大于50的低电平
	//HAL_Delay(1);	
	
	SPI2_WriteByte(gWs2812bDat_SPI,sizeof(gWs2812bDat_SPI));
	SPI2_WriteByte(&dat,1);
	
}
*/

//===================================================================================
/*
//将灯条显存SPI数据缓存
u8 gWs2812bDat_SPI[WS2812B_AMOUNT * 24] = {0};	//灯珠要发送数据数组
//灯条显存
tWs2812bCache_TypeDef gWs2812bDat[WS2812B_AMOUNT] = {
//R    G      B
0X0F, 0X0F, 0X0F,	//0
0X0F, 0X0F, 0X0F,	//1
0X0F, 0X0F, 0X0F,	//2
0X0F, 0X0F, 0X0F,	//3
0X0F, 0X0F, 0X0F,	//4
0X0F, 0X0F, 0X0F,	//5
0X0F, 0X0F, 0X0F,	//6
0X0F, 0X0F, 0X0F,	//7
};

//===================================================================================
void WS2812b_Set(uint16_t Ws2b812b_NUM, uint8_t r,uint8_t g,uint8_t b)
{
	u8 i;
	//根据灯珠颜色调整顺序
	uint8_t *pR = &gWs2812bDat_SPI[(Ws2b812b_NUM) * 24 + 8];  //灯珠颜色赋值
	uint8_t *pG = &gWs2812bDat_SPI[(Ws2b812b_NUM) * 24];
	uint8_t *pB = &gWs2812bDat_SPI[(Ws2b812b_NUM) * 24 + 16];
	
	
	//判断灯珠每个颜色字节每位状态，每个状态位赋值给一个字节发出，组成一个位，每个灯珠颜色有64个位
	//组成，每8个位组成灯珠颜色字节的一个位，需要控制SPI通讯速率
	for(i = 0; i < 8; i++) 
	{
		if(g & 0x80)     // 判断灯珠位开关，赋值给映射的一个字节
			{
			*pG = CODE_1;   //0xF8  1111 1000
		  }           
		else 
			{           
			*pG = CODE_0;   //0xE0  1110 0000
		  }           
		if(r & 0x80) 
			{           
			*pR = CODE_1;
		  }           
		else 
			{           
			*pR = CODE_0;
		  }           
		if(b & 0x80) 
			{           
			*pB = CODE_1;
		  }           
		else 
			{           
			*pB = CODE_0;
		  }
		  r <<= 1;  //移位判断每个颜色的位状态
		  g <<= 1;
	  	b <<= 1;
		  pR++;      //指针增加，增加每字节对应一个位。
		  pG++;
		  pB++;
	}
}

//===================================================================================
void WS2812B_Task(void)
{
	u8 dat = 0;
	u8 iLED;
	//解析WS2814数据   //WS2812B_AMOUNT   8
	for( iLED = 0; iLED < WS2812B_AMOUNT; iLED++) //
	{
		//把灯珠颜色状态写入到gWs2812bDat_SPI发送数组
		WS2812b_Set(iLED, gWs2812bDat[iLED].R, gWs2812bDat[iLED].G, gWs2812bDat[iLED].B);
	}
	//总线输出数据
	//HAL_SPI_Transmit(&hspi1, gWs2812bDat_SPI, sizeof(gWs2812bDat_SPI), 100);
	//使总线输出低电平
//	HAL_SPI_Transmit(&hspi1, &dat, 1, 100);
	//帧信号，一个大于50的低电平
	//HAL_Delay(1);	
	
	SPI2_WriteByte(gWs2812bDat_SPI,sizeof(gWs2812bDat_SPI));
	
	SPI2_WriteByte(&dat,1);
	
	
}

*/


