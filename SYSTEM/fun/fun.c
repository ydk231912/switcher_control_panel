#include "fun.h"
#include "tim.h"
#include "dma.h"
#include "led.h"
#include "usart.h"
#include "spi.h"
#include "can.h"
#include "oled.h"
#include "delay.h"
#include "bmp.h"
#include "dma.h"
#include "rgbw.h"
#include "can.h"
#include "adc.h"
#include "stm32f10x.h"


u8 Tx1Buf[DMATX1];  			//串口通讯
u8 Rx1Buf[DMATX1];
u8 Tx2Buf[DMATX2];
u8 Rx2Buf[DMATX2];
u8 Tx3Buf[DMATX3];
u8 Rx3Buf[DMATX3];

u8 Spi3TxBuf[SPI3FH1len];   	//SPI通讯
u8 Spi3RxBuf[SPI3FH1len];
u8 Spi3tempRxBuf[SPI3FH1len];

signed char keytim[25]={0};  	//按键延时--去抖
u8  candata[8];
u8  KeyBYTE[8]={0};
u16  temp1,temp5;
u8 USART2summRXold=0,USART2summRXniu;
u8 RGBWon=0;

// 定义常量
//#define DEBOUNCE_TIME_US 200 // 去抖动时间设置为200微秒

//===================================================================================
//修改电阻3.3K

void fun(void)
{
//===================================================================================
//	RGBW驱动
//	rgbw2814[0]=1;rgbw2814[1]=2;rgbw2814[2]=3;  rgbw2814[2]=3;rgbw2814[9]=3;rgbw2814[24]=3;
	
	if((rgbwtim>=50)&&(1))  	//发送RGBW灯光
	{ 
		rgbwtim=0;				// 重置计时器变量rgbwtim为0
		if(RGBWon==37)			//是否开启RGBW
		{
			RGBWon=0;			// 将RGBWon重置为0
			rgbwtim=0;			// 重置计时器变量rgbwtim为0
			WS2812B_Task();  	//RGBW按键
		}
	}

//============================================================================		
//can通信
	if(cantim>=1)
	{
		cantim=0;
		CAN1_Send_Num(0x602,candata);
	}
	candata[0]=KeyBYTE[0];candata[1]=KeyBYTE[1];candata[2]=KeyBYTE[2];candata[3]=KeyBYTE[3];
	candata[5]=ADCsim1; candata[4]=ADCsim1>>8;

//===================================================================================
//串口通信
	USART1_RD();
	USART2_RD();
// 	USART3_RD();
//	SPI3_RD();
	
	key();  //按键
	Get_Adc_Average1(10); 				//模拟量采集

//===================================================================================
//上位机传入的数据直接发送到从站，截取数据时Tx2Buf要发送到从站的数据
	OLEDtinct(Rx2Buf,rgbw2814,0,25);	//截取RGB显示数据
	temp1++;
	if(temp1>=100){LED2=!LED2;temp1=0;temp5++;	}	

	if(temp5>=200)temp5=0;
	oleddata=100-ADCsim1/40;			//推子模拟量
}

//===================================================================================
//截取OLED灯光显示
void OLEDtinct(u8 *USART,u8 *oled,u8 addr,u8 len)//截取OLED灯光显示
{
	u8 i,j=0;
	
	for(i=addr;i<len+addr;i++)
	{
		oled[j]=USART[i];
		j++;
	}
}

//==================与RAM主控板通讯===================================================
void USART1_RD(void)
{

	if(USART1_RxOV>=1)
	{
		USART1_RxOV=0;
		if((ucByteBufRX1[0]==0xA8)&&(ucByteBufRX1[1]==0x01))  	//判断是否是写OLED命令
		{		
			ucByteBufRX1[0]=0; ucByteBufRX1[1]=0;
			if((sumc(ucByteBufRX1,DMARX1C-1))==ucByteBufRX1[DMARX1C-1])
			{
			 //if(ucByteBufRX1[1]==0x01)  						//判断地址是显示屏还是数据
				{
					Rx_summ(Tx3Buf, ucByteBufRX1,DMARX1C-1);	//串口1接收到数据写入到串口3发送
				}
			}			
		} 
	} 
}

//===================发送按键灯光命令===============================================
void USART2_RD(void)
{
	/*
	//主机串口2与从机串口1通讯，发送按键灯光命令
	if((usart2time1>=20)||(usart2time2>=1))  					//发送数据
	{	
		ucByteBufTX2[DMATX2C-1]=Tx_summ(ucByteBufTX2, Tx2Buf,DMATX2C-1);	//累加和
		usart2time1=0; USART2_TxOV=0; usart2time2=0;  			//清零标志位
		ucByteBufTX2[0]=TFH2;                         			//帧头
		
		DMA_USART2_Tx(DMA1_Channel7,DMATX2C);		      		//发送数据		
  }
*/
	//串口1接收OLED数据，推子板没有OLED屏数据不处理		
	if(USART2_RxOV>=1)  //接收按键灯光命令
	{
		USART2_RxOV=0;    
		//if(ucByteBufRX2[0]==RFH2)  //判断帧�
		if((ucByteBufRX2[0]==0xB7)&&(ucByteBufRX2[1]==0x01))
		{	
			ucByteBufRX2[0]=0; 
			USART2summRXniu=crc16(ucByteBufRX2, DMARX2C);  //判断CRC
			if((sumc(ucByteBufRX2,DMARX2C-1))==ucByteBufRX2[DMARX2C-1])
			{
		
				Rx_summ(Rx2Buf, ucByteBufRX2,DMARX2C-1);	//校验正确赋值给数组
				if(USART2summRXniu!=USART2summRXold)  		//判断累加和是否相同，开启OLED显示
				{
					RGBWon=37;								//开启RGBW
				}
				USART2summRXold=USART2summRXniu;  			//赋值给旧值
				ucByteBufRX2[DMARX2C-1]=0;
			}			 
		}		
	}  
}

//====================发送OLED显示数据命令====================================
//485串口发送数据到从机，发送OLED显示屏数据 -- !!!目前没用到
void USART3_RD(void)
{
/*
	R485_1=1;  
	if((usart3time1>=20)||(usart3time2>=1))  //发送数据
	{
		usart3time2=0; usart3time1=0; USART3_TxOV=0;		
		ucByteBufTX3[DMATX3C-1]=Tx_summ(ucByteBufTX3, Tx3Buf,DMATX3C-1);	//累加和
		ucByteBufTX3[0]=TFH3;
		DMA_USART3_Tx(DMA1_Channel2,DMATX3C);		 //发送数据	
	}
		
// 	if(USART3_RxOV>=1)   //485串口3暂时不发数据
// 	{
// 		USART3_RxOV=0;
// 	
// 		if(ucByteBufRX3[0]==TFH2)  //判断帧头
// 		{		
// 		 	ucByteBufRX3[0]=0;
// 		 	if((cumulative_sumc(ucByteBufRX3,DMARX3C-1))==ucByteBufRX3[DMARX3C-1])
// 		 	{
// 				cumulative_summ(Rx3Buf, ucByteBufRX3,DMARX3C-1);
//      	}			
//  	} 	
//  } */
}

//=================发送累加交换校验和==========================================================
//cumulative_summ(ucByteBufTX2, Tx2Buf,DMATX2C-1)
//校验去除帧头加地址两个字节 发送校验
u16 Tx_summ(volatile u8 *Buf1, volatile u8 *Buf2,u16 len) //累加和校验
{
	u16 sumdata=0;
	u16 i;
	u16 j=0;
	for(i=2;i<len;i++)
	{
		Buf1[j+2]=Buf2[j];	//发送字符串
		sumdata+=Buf2[j];
		j++;
	}		
	return  (sumdata&0x00ff); //一个字节累加和
}

//===========接收校验===========================================================	
////校验去除帧头加地址两个字节  接收校验
u16 Rx_summ(volatile u8 *Buf1, volatile u8 *Buf2,u16 len) //累加和校验
{
	u16 sumdata=0;
	u16 i;
	u16 j=0;
	
	for(i=1;i<len;i++)
	{
		Buf1[j]=Buf2[j+2];	
		sumdata+=Buf2[j+2];
		j++;
	}	
	return  (sumdata&0x00ff); //一个字节累加和
}
	
//==============累加校验和================================================================
u16 sumc(volatile u8 *Buf1, u16 len) //累加和校验
{
	u16 sumdata=0;
	u16 i;
	for(i=2;i<len;i++)
	{	 
		sumdata+=Buf1[i];	
	}		
	return  (sumdata&0x00ff); //一个字节累加和
}	

//===================================================================================
void interrupt(u8 bit)
{
	if(bit)
	{
		USART1->CR1&=~(1<<4);
		USART2->CR1&=~(1<<4);
		USART3->CR1&=~(1<<4);
		DMA1_Channel4->CCR&=~(1<<0);
		DMA1_Channel7->CCR&=~(1<<0);	
		DMA1_Channel2->CCR&=~(1<<0);	
	}
	else
	{
		USART1->CR1|=1<<4;
		USART2->CR1|=1<<4;
		USART3->CR1|=1<<4;
		DMA1_Channel4->CCR|=1<<0;
		DMA1_Channel7->CCR|=1<<0;
		DMA1_Channel2->CCR|=1<<0;
	}
}	

//===================================================================================
//// 按键状态定义
//typedef enum {
//	KEY_IDLE,    // 按键空闲状态
//	KEY_PRESSED  // 按键已按下状态
//} KeyState;
//volatile KeyState keyState = KEY_IDLE;  // 当前按键状态
//volatile uint32_t lastPressTime = 0;    // 上次按键时间（计数器值）
//volatile uint8_t lastKeyState = 0;      // 上一次按键状态

void key(void)
{
//test key one send date?
//	uint32_t currentTime = TIM2->CNT; 		// 获取当前定时器计数值
//	
//	if(key1 != lastKeyState){
//		lastPressTime = currentTime;
//        lastKeyState = key1;
//	}
//	if(key1 && (currentTime - lastPressTime >= DEBOUNCE_TIME_US)){
//		if (keyState == KEY_IDLE){
//			KeyBYTE[0]&=~(1<<0);
//			keyState = KEY_PRESSED; // 更新按键状态为已按下
//		}
//    } else if(!key1){
//		keyState = KEY_IDLE; // 按键释放，更新状态为空闲
//	}
	
	
	if(key1){ if(keytim[0]<keycon) keytim[0]++;  else   KeyBYTE[0]&=~(1<<0);} //1
      else { if(keytim[0]>keycon)keytim[0]--;  else   KeyBYTE[0]|=0x01; } 
	
	if(key2){ if(keytim[1]<keycon) keytim[1]++;  else   KeyBYTE[0]&=~(1<<1);} //2
      else { if(keytim[1]>keycon)keytim[1]--;  else   KeyBYTE[0]|=0x02;} 
	
	if(key3){ if(keytim[2]<keycon) keytim[2]++;  else   KeyBYTE[0]&=~(1<<2);} //3
      else { if(keytim[2]>keycon)keytim[2]--;  else   KeyBYTE[0]|=0x04;} 
	
	if(key4){ if(keytim[3]<keycon) keytim[3]++;  else   KeyBYTE[0]&=~(1<<3);} //4
      else { if(keytim[3]>keycon)keytim[3]--;  else   KeyBYTE[0]|=0x08;} 
	
	if(key5){ if(keytim[4]<keycon) keytim[4]++;  else   KeyBYTE[0]&=~(1<<4);} //5
      else { if(keytim[4]>keycon)keytim[4]--;  else   KeyBYTE[0]|=0x10;} 
	
	if(key6){ if(keytim[5]<keycon) keytim[5]++;  else   KeyBYTE[0]&=~(1<<5);} //6 
      else { if(keytim[5]>keycon)keytim[5]--;  else   KeyBYTE[0]|=0x20;} 
	
	if(key7){ if(keytim[6]<keycon) keytim[6]++;  else   KeyBYTE[0]&=~(1<<6);} //7
      else { if(keytim[6]>keycon)keytim[6]--;  else   KeyBYTE[0]|=0x40;} 		
					
	if(key8){ if(keytim[7]<keycon) keytim[7]++;  else   KeyBYTE[0]&=~(1<<7);} //8
      else { if(keytim[7]>keycon)keytim[7]--;  else   KeyBYTE[0]|=0x80;} 		
				
	if(key9){ if(keytim[8]<keycon) keytim[8]++;  else   KeyBYTE[1]&=~(1<<0);} //9
      else { if(keytim[8]>keycon)keytim[8]--;  else   KeyBYTE[1]|=0x01; }		
			
	if(key10){ if(keytim[9]<keycon) keytim[9]++; else    KeyBYTE[1]&=~(1<<1);} 	 //10
      else { if(keytim[9]>keycon)keytim[9]--;  else    KeyBYTE[1]|=0x02;} 		
			
	if(key11){ if(keytim[10]<keycon) keytim[10]++; else    KeyBYTE[1]&=~(1<<2);} //11
      else { if(keytim[10]>keycon)keytim[10]--;  else    KeyBYTE[1]|=0x04;} 		
		
	if(key12){ if(keytim[11]<keycon) keytim[11]++; else    KeyBYTE[1]&=~(1<<3);} //12
      else { if(keytim[11]>keycon)keytim[11]--;  else    KeyBYTE[1]|=0x08;}

	if(key13){ if(keytim[12]<keycon) keytim[12]++;  else   KeyBYTE[1]&=~(1<<4);} //13
      else { if(keytim[12]>keycon)keytim[12]--;   else   KeyBYTE[1]|=0x10;}		
		
	if(key14){ if(keytim[13]<keycon) keytim[13]++;  else   KeyBYTE[1]&=~(1<<5);} //14 
      else { if(keytim[13]>keycon)keytim[13]--;   else   KeyBYTE[1]|=0x20;} 
	
	if(key15){ if(keytim[14]<keycon) keytim[14]++;  else   KeyBYTE[1]&=~(1<<6);} //15
      else { if(keytim[14]>keycon)keytim[14]--;   else   KeyBYTE[1]|=0x40;} 		
					
	if(key16){ if(keytim[15]<keycon) keytim[15]++;  else   KeyBYTE[1]&=~(1<<7);} //16
      else { if(keytim[15]>keycon)keytim[15]--;   else   KeyBYTE[1]|=0x80;} 		
	  
	if(key17){ if(keytim[16]<keycon) keytim[16]++;  else   KeyBYTE[2]&=~(1<<0);} //17
      else { if(keytim[16]>keycon)keytim[16]--;  else   KeyBYTE[2]|=0x01; }		
			
	if(key18){ if(keytim[17]<keycon) keytim[17]++; else    KeyBYTE[2]&=~(1<<1);} //18
      else { if(keytim[17]>keycon)keytim[17]--;  else    KeyBYTE[2]|=0x02;} 		
			
	if(key19){ if(keytim[18]<keycon) keytim[18]++; else    KeyBYTE[2]&=~(1<<2);} //19
      else { if(keytim[18]>keycon)keytim[18]--;  else    KeyBYTE[2]|=0x04;} 		
		
	if(key20){ if(keytim[19]<keycon) keytim[19]++; else    KeyBYTE[2]&=~(1<<3);} //20
      else { if(keytim[19]>keycon)keytim[19]--;  else    KeyBYTE[2]|=0x08;}

	if(key21){ if(keytim[20]<keycon) keytim[20]++;  else   KeyBYTE[2]&=~(1<<4);} //21
      else { if(keytim[20]>keycon)keytim[20]--;   else   KeyBYTE[2]|=0x10;}		
		
	if(key22){ if(keytim[21]<keycon) keytim[21]++;  else   KeyBYTE[2]&=~(1<<5);} //22 
      else { if(keytim[21]>keycon)keytim[21]--;   else   KeyBYTE[2]|=0x20;} 
	
	if(key23){ if(keytim[22]<keycon) keytim[22]++;  else   KeyBYTE[2]&=~(1<<6);} //23
      else { if(keytim[22]>keycon)keytim[22]--;   else   KeyBYTE[2]|=0x40;} 		
					
	if(key24){ if(keytim[23]<keycon) keytim[23]++;  else   KeyBYTE[2]&=~(1<<7);} //24
      else { if(keytim[23]>keycon)keytim[23]--;   else   KeyBYTE[2]|=0x80;} 
			
	if(key25){ if(keytim[24]<keycon) keytim[24]++;  else   KeyBYTE[3]&=~(1<<0);} //25
      else { if(keytim[24]>keycon)keytim[24]--;   else   KeyBYTE[3]|=0x01;} 				
}

//=====================================================================
void SPI3_RD(void)
{
	//主机SPI与从机SPI1通讯  SPIFH1
	if(SPItim>=100)
	{
		SPItim=0;
	
		Spi3TxBuf[SPI3FH1len-1]=sumc(Spi3TxBuf, SPI3FH1len-1);  //发送数据校验累加和
		Spi3TxBuf[0]=SPI3FH1;									//帧头
		
//		SPI3_ReadorWriteByte(Spi3TxBuf,Spi3RxBuf,SPI3FH1len); 	//与从机通信 
	
		if(Spi3tempRxBuf[0]==SPI3FH1) 							//判断帧头
		{
			Spi3tempRxBuf[0]=0;
	//		if((cumulative_sumc(Spi3tempRxBuf,SPI3FH1len-1))==Spi3tempRxBuf[SPI3FH1len-1])  //判断接收数据累加和
			{
				Tx_summ(Spi3RxBuf, Spi3tempRxBuf,SPI3FH1len-1);			
			}	
		}
	}	 

}

//===================================================================================
const unsigned char   auchCRCHi[] =
{
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
    0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
    0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
    0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
    0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
    0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
    0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
    0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
    0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
    0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40
} ;

//===================================================================================
/* CRC低位字节值表*/
const unsigned char   auchCRCLo[] =
{
    0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06,
    0x07, 0xC7, 0x05, 0xC5, 0xC4, 0x04, 0xCC, 0x0C, 0x0D, 0xCD,
    0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09,
    0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A,
    0x1E, 0xDE, 0xDF, 0x1F, 0xDD, 0x1D, 0x1C, 0xDC, 0x14, 0xD4,
    0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,
    0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3,
    0xF2, 0x32, 0x36, 0xF6, 0xF7, 0x37, 0xF5, 0x35, 0x34, 0xF4,
    0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A,
    0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38, 0x28, 0xE8, 0xE9, 0x29,
    0xEB, 0x2B, 0x2A, 0xEA, 0xEE, 0x2E, 0x2F, 0xEF, 0x2D, 0xED,
    0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
    0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60,
    0x61, 0xA1, 0x63, 0xA3, 0xA2, 0x62, 0x66, 0xA6, 0xA7, 0x67,
    0xA5, 0x65, 0x64, 0xA4, 0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F,
    0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68,
    0x78, 0xB8, 0xB9, 0x79, 0xBB, 0x7B, 0x7A, 0xBA, 0xBE, 0x7E,
    0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,
    0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71,
    0x70, 0xB0, 0x50, 0x90, 0x91, 0x51, 0x93, 0x53, 0x52, 0x92,
    0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C,
    0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B,
    0x99, 0x59, 0x58, 0x98, 0x88, 0x48, 0x49, 0x89, 0x4B, 0x8B,
    0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
    0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42,
    0x43, 0x83, 0x41, 0x81, 0x80, 0x40
} ;

//===================================================================================
/******************************************************************
功能: CRC16校验
输入: 指向待校验数据的指针 + 校验数据长度
输出: CRC计算结果
说明: 通用程序
******************************************************************/
unsigned int crc16(volatile unsigned char * puchMsg, unsigned int usDataLen)
{
    unsigned char uchCRCHi = 0xFF ; 				// 高CRC字节初始化
    unsigned char uchCRCLo = 0xFF ; 				// 低CRC 字节初始化
    unsigned long uIndex ; 							// CRC循环中的索引

    while (usDataLen--) 							// 传输消息缓冲区
    {
        uIndex = uchCRCHi ^ *puchMsg++ ; 			// 计算CRC
        uchCRCHi = uchCRCLo ^ auchCRCHi[uIndex] ;
        uchCRCLo = auchCRCLo[uIndex] ;
    }

    return (uchCRCHi << 8 | uchCRCLo) ;
}

//===================================================================================
/*
	
		OLED_DrawBMP(0,0,256,128,gImage_1,1);
		delay_ms(1000);
		OLED_Fill(0,0,256,128,0x00);
		OLED_ShowString(44,0,"2.70_Inch OLED",24,0);
		OLED_ShowString(14,30,"With 16 Gray Scales",24,0);
		OLED_ShowString(26,60,"DRIVER IC:SSD1363",24,0);
		OLED_ShowString(44,90,"PIXELS:256x128",24,0);
		delay_ms(1000);
		OLED_Fill(0,0,256,128,0x00);
		OLED_ShowChinese(88,15,"?????",16,0);
		OLED_ShowChinese(68,35,"?????",24,0);
		OLED_ShowChinese(48,65,"?????",32,0);
		delay_ms(1000);
		OLED_Fill(0,0,256,128,0x00);
		OLED_DrawSingleBMP(0,0,256,128,gImage_2,0);
		delay_ms(1000);
	
//===================================================================================
u16 cumulative_summ(volatile u8 *Buf1, volatile u8 *Buf2,u16 len) //累加和校验
{
	u16 sumdata=0;
	u16 i;
	
	for(i=1;i<len;i++)
	{
		*(Buf1+i)=*(Buf2+i);	
		sumdata+=*(Buf2+i);	
	}		
	return  (sumdata&0x00ff); //一个字节累加和
}
*/








