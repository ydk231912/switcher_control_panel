#ifndef __RGBW_H
#define __RGBW_H

#include "sys.h"



#define CODE_0		0xE0  //位
#define CODE_1		0xF8

#define WS2812B_AMOUNT		25   //灯数量
#define  Rbrigh       50   //RGBW亮度显示
#define  bRbrigh       2   //RGBW亮度显示
#define  gRbrigh       2   //RGBW亮度显示
#define  Gbrigh       50   //RGBW亮度显示
#define  WBbrigh      24   //RGBW亮度显示
#define  Bbrigh       50   //RGBW亮度显示
#define  Wbrigh       50   //RGBW亮度显示


//电流与占空比  //55=1.4ma    85=2ma   150=3.3ma   35=1ma
//

typedef struct
{
    uint8_t R;
    uint8_t G;
    uint8_t B;
    uint8_t W;
} tWs2812bCache_TypeDef;

extern tWs2812bCache_TypeDef gWs2812bDat[WS2812B_AMOUNT];

extern u8 rgbw2814[WS2812B_AMOUNT];  //灯控制数组



void WS2812b_Set(u16 Ws2b812b_NUM, u8 rgbw,u8 r, u8 br, u8 g, u8 wg, u8 b,u8 w ,u8 gr ,u8 temp);
void WS2812B_Task(void);

extern u8 gWs2812bDat_SPI[WS2812B_AMOUNT * 24];	//灯珠要发送数据数组

#endif

