#ifndef __RGBW_H
#define __RGBW_H	

#include "sys.h"



#define CODE_0		0xE0  //位
#define CODE_1		0xF8

#define WS2812B_AMOUNT		24   //灯数量

typedef struct
{
	uint8_t R;
	uint8_t G;
	uint8_t B;
	uint8_t W;
} tWs2812bCache_TypeDef;

extern tWs2812bCache_TypeDef gWs2812bDat[WS2812B_AMOUNT];

extern u8 rgbw2814[WS2812B_AMOUNT];  //灯控制数组



void WS2812b_Set(u16 Ws2b812b_NUM, u8 rgbw,u8 r, u8 g, u8 b,u8 w ,u8 temp);
void WS2812B_Task(void);

extern u8 gWs2812bDat_SPI[WS2812B_AMOUNT * 32];	//灯珠要发送数据数组

#endif

