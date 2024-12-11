#include "rgbw.h"
#include "spi.h"
#include "dma.h"
#include "delay.h"
//�������Դ�SPI���ݻ���
u8 gWs2812bDat_SPI[WS2812B_AMOUNT * 24] = {0};	//����Ҫ������������
//�����Դ�

u8 rgbw2814[WS2812B_AMOUNT]= {0}; //�ƿ�������



//��ɫ r�� g �� b �� w ��
void WS2812b_Set(u16 Ws2b812b_NUM, u8 rgbw,u8 r,u8 br, u8 g, u8 wb, u8 b, u8 w ,u8 gr,u8 temp)
{
    u8 i;

    //���ݵ�����ɫ����˳��
    u8 *pR = &gWs2812bDat_SPI[(Ws2b812b_NUM) * 24 + 8];  //������ɫ��ֵ
    u8 *pB = &gWs2812bDat_SPI[(Ws2b812b_NUM) * 24];
    u8 *pG = &gWs2812bDat_SPI[(Ws2b812b_NUM) * 24 + 16];
//    u8 *pB = &gWs2812bDat_SPI[(Ws2b812b_NUM) * 32 + 24];

    //�жϵ���ÿ����ɫ�ֽ�ÿλ״̬��ÿ��״̬λ��ֵ��һ���ֽڷ��������һ��λ��ÿ��������ɫ��64��λ
    //��ɣ�ÿ8��λ��ɵ�����ɫ�ֽڵ�һ��λ����Ҫ����SPIͨѶ����
    for(i = 0; i < 8; i++)
    {
		if(rgbw == 1) {		// ��ʾ��ɫ
			if ((b & 0x80)) *pB = CODE_1;
            else *pB = CODE_0;
			
			if ((r & 0x80)) *pR = CODE_1;
            else *pR = CODE_0;

            if ((g & 0x80)) *pG = CODE_1;
            else *pG = CODE_0;
		} else if (rgbw == 2) {	// ��ʾ��ɫ
			if ((r & 0x80)) *pR = CODE_1;
            else *pR = CODE_0;
			
			if ((br & 0x80)) *pB = CODE_1;
			else *pB = CODE_0;
//			*pB = CODE_0;
			
			*pG = CODE_0;
		} else if (rgbw == 4) { // ��ʾ��ɫ 
            if ((g & 0x80)) *pG = CODE_1;
            else *pG = CODE_0;

			*pR = CODE_0;
			*pB = CODE_0;
		} else if (rgbw == 3) {	// ��ʾ��ɫ
			if ((b & 0x80)) *pB = CODE_1;
            else *pB = CODE_0;
			if ((gr & 0x80)) *pR = CODE_1;
			else *pR = CODE_0;
			*pG = CODE_0;
		} else if (rgbw == 5) {  // ��ʾ��ɫ
            if ((r & 0x80)) *pR = CODE_1;
            else *pR = CODE_0;

            if ((wb & 0x80)) *pB = CODE_1;
            else *pB = CODE_0;

            *pG = CODE_0;
		}
        

        r<<= 1;    //��λ�ж�ÿ����ɫ��λ״̬
		br<<=1;
        g<<= 1;
		wb<<= 1;
		gr<<=1;
        b<<= 1;

        pR++;      //ָ�����ӣ�����ÿ�ֽڶ�Ӧһ��λ��
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
    //����WS2814����   //WS2812B_AMOUNT   8

    for( iLED = 0; iLED < WS2812B_AMOUNT; iLED++) //
    {
        rgbwtemp=rgbw2814[iLED]&0x07;
        WS2812b_Set(iLED,rgbwtemp ,Rbrigh,bRbrigh, Gbrigh, WBbrigh, Bbrigh, Wbrigh, gRbrigh,0);   //�ѵ�����ɫ״̬д�뵽gWs2812bDat_SPI��������
    }

    SPI2_WriteByte(gWs2812bDat_SPI,len);
    SPI2_WriteByte(&dat,2);

    //DMA_SPI2_Tx(gWs2812bDat_SPI,sizeof(gWs2812bDat_SPI));
    //DMA_SPI2_Tx(&dat,1);



}





