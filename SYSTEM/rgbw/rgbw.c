#include "rgbw.h"
#include "spi.h"
#include "dma.h"
#include "delay.h"
//将灯条显存SPI数据缓存
u8 gWs2812bDat_SPI[WS2812B_AMOUNT * 24] = {0};	//灯珠要发送数据数组
//灯条显存

u8 rgbw2814[WS2812B_AMOUNT]= {0}; //灯控制数组



//颜色 r红 g 绿 b 蓝 w 白
void WS2812b_Set(u16 Ws2b812b_NUM, u8 rgbw,u8 r,u8 br, u8 g, u8 wb, u8 b, u8 w ,u8 gr,u8 temp)
{
    u8 i;

    //根据灯珠颜色调整顺序
    u8 *pR = &gWs2812bDat_SPI[(Ws2b812b_NUM) * 24 + 8];  //灯珠颜色赋值
    u8 *pB = &gWs2812bDat_SPI[(Ws2b812b_NUM) * 24];
    u8 *pG = &gWs2812bDat_SPI[(Ws2b812b_NUM) * 24 + 16];
//    u8 *pB = &gWs2812bDat_SPI[(Ws2b812b_NUM) * 32 + 24];

    //判断灯珠每个颜色字节每位状态，每个状态位赋值给一个字节发出，组成一个位，每个灯珠颜色有64个位
    //组成，每8个位组成灯珠颜色字节的一个位，需要控制SPI通讯速率
    for(i = 0; i < 8; i++)
    {
		if(rgbw == 1) {		// 显示白色
			if ((b & 0x80)) *pB = CODE_1;
            else *pB = CODE_0;
			
			if ((r & 0x80)) *pR = CODE_1;
            else *pR = CODE_0;

            if ((g & 0x80)) *pG = CODE_1;
            else *pG = CODE_0;
		} else if (rgbw == 2) {	// 显示红色
			if ((r & 0x80)) *pR = CODE_1;
            else *pR = CODE_0;
			
			if ((br & 0x80)) *pB = CODE_1;
			else *pB = CODE_0;
//			*pB = CODE_0;
			
			*pG = CODE_0;
		} else if (rgbw == 4) { // 显示蓝色 
            if ((g & 0x80)) *pG = CODE_1;
            else *pG = CODE_0;

			*pR = CODE_0;
			*pB = CODE_0;
		} else if (rgbw == 3) {	// 显示绿色
			if ((b & 0x80)) *pB = CODE_1;
            else *pB = CODE_0;
			if ((gr & 0x80)) *pR = CODE_1;
			else *pR = CODE_0;
			*pG = CODE_0;
		} else if (rgbw == 5) {  // 显示橙色
            if ((r & 0x80)) *pR = CODE_1;
            else *pR = CODE_0;

            if ((wb & 0x80)) *pB = CODE_1;
            else *pB = CODE_0;

            *pG = CODE_0;
		}
        

        r<<= 1;    //移位判断每个颜色的位状态
		br<<=1;
        g<<= 1;
		wb<<= 1;
		gr<<=1;
        b<<= 1;

        pR++;      //指针增加，增加每字节对应一个位。
        pG++;
        pB++;

    }
}
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
        rgbwtemp=rgbw2814[iLED]&0x07;
        WS2812b_Set(iLED,rgbwtemp ,Rbrigh,bRbrigh, Gbrigh, WBbrigh, Bbrigh, Wbrigh, gRbrigh,0);   //把灯珠颜色状态写入到gWs2812bDat_SPI发送数组
    }

    SPI2_WriteByte(gWs2812bDat_SPI,len);
    SPI2_WriteByte(&dat,2);

    //DMA_SPI2_Tx(gWs2812bDat_SPI,sizeof(gWs2812bDat_SPI));
    //DMA_SPI2_Tx(&dat,1);



}





