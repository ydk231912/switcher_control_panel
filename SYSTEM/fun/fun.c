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
#include "rgbw.h"
#include "rgbw.h"


u8 Tx1Buf[DMATX1];  //´®¿ÚÍ¨Ñ¶
u8 Rx1Buf[DMARX1];
u8 Tx2Buf[DMATX2];
u8 Rx2Buf[DMARX2];
u8 Tx3Buf[DMATX3];
u8 Rx3Buf[DMARX3];


//Ò»¸öÆÁ·ÖÁ½´ÎÊý¾Ý·¢ËÍ
u8 oled1[oledlen]={0};
u8 oled2[oledlen]={0};
u8 oled3[oledlen]={0};
u8 oled4[oledlen]={0};
u8 oled5[oledlen]={0};
u8 oled6[oledlen]={0};


u8  USART1summRXold=0,USART1summRXniu;
u8  USART2summRXold=0,USART2summRXniu;
u8  oledon=0;         //¿ªÆôOLEDÆÁË¢ÐÂÊý¾Ý
u8  oledonDelay=0;    //OLEDÑÓÊ±¶Ï¿ª 
u8  OLEDorderonly=0;  //OLEDÏÔÊ¾Ë³Ðò£¬Ò²ÊÇ
u16 OLEDmultiple=0;   //±¾»úOLEDÏÔÊ¾Ë³Ðò±¶Êý

u8  RGBWDelay=0;      //RGBWÑÓÊ±¶Ï¿ª
u8  RGBWmultiple=0;   //RGBW½ØÈ¡Ë³Ðò
u8  RGBWon=0;         //RGBW¿ª¹ØË¢ÐÂÊý¾Ý

signed char keytim[24]={0};  //°´¼üÑÓÊ±
signed char ktim[4]={0};  //°´¼üÑÓÊ±

u8  candata[8];
u8  KeyBYTE[8]={0};
u16 canno=0;          //ÑÓÊ±Æô¶¯CAN
u16 CANaddr=0;
u8  CANkey=0;           //CAN¿ª¹Ø
u8  CAN1_SendC=0;      //·¢ËÍÊ§°Ü¼ÆÊý
u8  CAN1_keycount=0;   //°´¼ü·¢ËÍ³É¹¦ºóÑÓÊ±¶Ï¿ª
u8  CAN1_sw=0;        //can¿ª¹Ø
u16 temp1;
u8 CAN1times=0;       //CAN¶¨Ê±·¢ËÍÊ±¼ä

void fun(void)
{	
//==========================================================
	//½ØÈ¡RGBWÏÔÊ¾Êý¾Ý//×¢Òâ½ØÈ¡Ê±´ÓÊý×éµÚÒ»Î»¿ªÊ¼£¬Êý×éÒÑ¾­ÕûºÏÊý¾Ý£

	if((rgbwtim>=10)&&(1))  //·¢ËÍRGBWµÆ¹â
	{ 
		OLEDtinct(Rx2Buf,rgbw2814,RGBWmultiple,24);
		rgbwtim=0;
		WS2812B_Task(); 
		if(RGBWon==59)
		{ 
			RGBWDelay++; 
		//WS2812B_Task();  //RGBW°´¼ü CANaddr	
			if(RGBWDelay>=3)  //·¢ËÍ3´Îºó¶Ï¿ªRGBWÊý¾Ý
			{
			 RGBWon=0;
		   rgbwtim=0;   	
			}
		}
	}
	//=================================================
	IWDG_Feed();    //=======================
	key();  //°´¼ü
	candata[0]=KeyBYTE[0];candata[1]=KeyBYTE[1];candata[2]=KeyBYTE[2];
//=======================CANÍ¨ÐÅÊý¾Ý================================================

if(cantim>200)cantim=210;
if(CAN1_keycount>100)	CAN1_keycount=0;
	
 #if U1orU2	
 if(((CANkey>0)&&(CANkey<=24)))  //°´¼ü·¢ËÍCANÃüÁî
 {
	 CAN1_sw=1;
	 CAN1_keycount=0;
	 CANkey=0;
 }
	else if(CAN1_keycount>=30) //·¢ËÍ50Ö¡Êý¾Ý
	{
		CANkey=0;
		CAN1_sw=0;
		CAN1_keycount=0;
  }
	else;	
	if(CAN1_keycount>150)CAN1_keycount=200;
	
 #else
	CAN1_sw=0;
  CAN1times=3;
 #endif	
	
//=========================================
	#if U1orU2
	if((CAN1_sw)&&(cantim>=2))  //°´ÏÂ°´¼üÊ±Ò²ÒªµÈ´ý2mm·¢ËÍ£¬·ÀÖ¹Êý¾ÝÓµ¶Â£¬Èç²»ÐèÒª£¬¿ÉÐ´Èë1
	#else
	if((CAN1_sw)||(cantim>=CAN1times))
	#endif	
	{
		CAN1_keycount++;
		cantim=0;
		if(CAN1_Send_Num(CANaddr,candata))  //ÅÐ¶Ï·¢ËÍÊÇ·ñÕýÈ·
		{
			CAN1_SendC++;                   //Èç¹û·¢ËÍ´íÎó³¬¹ýÁ½´Î£¬³õÊ¼»¯CAN£
		}	
		else 
		 { 	 
			CAN1_SendC=0;       //ÇåÁãCAN·¢ËÍ´íÎó¼ÆÊý		
		 }
		 
		if((CAN1_SendC>5)||(EPV_F>=1)||(BOF_F>=1)) //
		{
			CAN1_SendC=0;
		  CAN1_Mode_Init(1,3,2,6,0); //CAN³õÊ¼»¯
		}
	}
	
//===============================================
	 USART1_RD();
   USART2_RD(); 
//=============================================================
	//OLEDÏÔÊ¾Êý¾Ý
	
	if(oledon==37)
	{			
		oleddata(Rx1Buf,oled1, OLEDmultiple+0);
		oleddata(Rx1Buf,oled2, OLEDmultiple+20);
		
		oleddata(Rx1Buf,oled3, OLEDmultiple+40);
		oleddata(Rx1Buf,oled4, OLEDmultiple+60);
		
		oleddata(Rx1Buf,oled5, OLEDmultiple+80);
		oleddata(Rx1Buf,oled6, OLEDmultiple+100);
		CSse=0;
    OLED_ShowString(12,10,oled1,16,0);
		OLED_ShowString(12,32,oled2,16,0);
		CSse=1;
    OLED_ShowString(12,10,oled3,16,0);
		OLED_ShowString(12,32,oled4,16,0);
		CSse=2;
    OLED_ShowString(12,10,oled5,16,0);
		OLED_ShowString(12,32,oled6,16,0);
		
		oledonDelay++;
		
    if(oledonDelay>3) //ÏÔÊ¾ÆÁË¢ÐÂ3´Îºó¶Ï¿ª
		{
			oledon=0;
			oledonDelay=0;
	//	OLEDmultiple=0;
    }
	  IWDG_Feed();//===========
	}

//===========================================================
	//Íâ²¿Ö¸Ê¾µÆ
	temp1++;
	if(temp1>=1000){LED2=!LED2;temp1=0;	}	//ÔËÐÐÖ¸Ê¾
}

//============================================================
//===============================================================
//²¦Âë¿ª¹ØµØÖ·Ñ¡Ôñ
void address_selection(void)
{
	//¸ù¾Ý²¦Âë¿ª¹Ø¼ÆËãOLEDÏÔÊ¾ÆÁµ±Ç°½ØÈ¡µÄÊý¾Ý
	OLEDorderonly=0;
	
	if(k1==0) OLEDorderonly|=0x01; else OLEDorderonly&=~(1<<0);
	
	if(k2==0) OLEDorderonly|=0x02; else OLEDorderonly&=~(1<<1);
	
	if(k3==0) OLEDorderonly|=0x04; else OLEDorderonly&=~(1<<2); 
	
	if(k4==0) OLEDorderonly|=0x08; else OLEDorderonly&=~(1<<3);
		
	
	if(OLEDorderonly==1)     { OLEDmultiple=0;    CANaddr=0x604;RGBWmultiple=25;canno=30;}   //µÚÒ»¸öÆÁ
	
	else if(OLEDorderonly==2){ OLEDmultiple=120;   CANaddr=0x606;RGBWmultiple=49;canno=40;}  //µÚ¶þ¸öÆÁ
	
	else if(OLEDorderonly==3){ OLEDmultiple=240;  CANaddr=0x608;RGBWmultiple=73;canno=50;} //µÚÈý¸öÆÁ
	
	else if(OLEDorderonly==4){ OLEDmultiple=360;  CANaddr=0x610;RGBWmultiple=97;canno=60;} //µÚËÄ¸öÆÁ
	
	else if(OLEDorderonly==5){ OLEDmultiple=480;  CANaddr=0x612;RGBWmultiple=121;canno=70;} //µÚÎå¸öÆÁ
	
	else if(OLEDorderonly==6){ OLEDmultiple=600;  CANaddr=0x614;RGBWmultiple=145;canno=80;}//µÚÁù¸öÆÁ
	
	else { OLEDmultiple=0;  CANaddr=0x500;RGBWmultiple=1;}
	
}

//============================================================
//OLEDÏÔÊ¾ ÌáÈ¡Ò»Ö¡Êý¾Ý
void oleddata(u8*Rx1Beye,u8*oledbeye, u16 addr)
	{
		u16 i; 
		u16 n=0;
		u16 j=addr;
		
		for(i=addr;i<addr+20+12;i++)
		{
			if(n<5)
			oledbeye[n++]=Rx1Beye[j++];   //µÚÒ»×éÊý¾Ý
			
			if((n>=5)&&(n<8))             //µÚÒ»¿Õ¸ñ
			oledbeye[n++]=0x20;
		
			if((n>=8)&(n<13))              //µÚ¶þ×éÊý¾Ý
			oledbeye[n++]=Rx1Beye[j++];	
			
			if((n>=13)&&(n<16))            //µÚ¶þ¿Õ¸ñ
			oledbeye[n++]=0x20;
			
			if((n>=16)&&(n<21))            //µÚÈý×éÊý¾Ý
			oledbeye[n++]=Rx1Beye[j++];	
			
			if((n>=21)&&(n<24))           //µÚÈý¿Õ¸ñ
			oledbeye[n++]=0x20;
			
 			if((n>=24)&&(n<29))           //µÚÈý×éÊý¾Ý
 			oledbeye[n++]=Rx1Beye[j++];	

    }
  }
	
//============================================================u8 
//½ÓÊÕÖ÷Õ¾·¢ËÍOLEDÊý¾ÝUSART1summRXold=0,USART2summRXold,USART1summRXniu,USART2summRXniu;
void USART1_RD(void)
{
	R485_2=0;
	
	//ÅÐ¶ÏÒ»Ö¡Êý¾ÝCRCÐ£Ñé½á¹û£¬Èç¹û½á¹ûÏàÍ¬²»Ë¢ÐÂOLED£¬Èç¹û²»ÏàÍ¬Ë¢ÐÂOLED
  if(USART1_RxOV>=1)
	{
		USART1_RxOV=0;
	if((ucByteBufRX1[0]==0xA8)&&(ucByteBufRX1[1]==0x01))
		{		
		 ucByteBufRX1[0]=0;
		 USART1summRXniu=crc16(ucByteBufRX1, DMARX1C);  //ÅÐ¶ÏCRC
		 if(sumc(ucByteBufRX1,DMARX1C-1)==ucByteBufRX1[DMARX1C-1])
		 {
			Rx_summ(Rx1Buf, ucByteBufRX1,DMARX1C-1);
      if(USART1summRXniu!=USART1summRXold)  //ÅÐ¶ÏÀÛ¼ÓºÍÊÇ·ñÏàÍ¬£¬¿ªÆôOLEDÏÔÊ¾
			{
				oledon=37;	//¿ªÆôOLED
      }
			 USART1summRXold=USART1summRXniu;  //¸³Öµ¸ø¾ÉÖµ
			 ucByteBufRX1[DMARX1C-1]=0;
     }			
    } 	
  } 
}
//=================================================================
void USART2_RD(void)
{
  R485_1=0;
	if(USART2_RxOV>=1)  //½ÓÊÕ°´¼üµÆ¹âÃüÁî
	{
		USART2_RxOV=0;    
		//if(ucByteBufRX2[0]==RFH2)  //ÅÐ¶ÏÖ¡Í
		if((ucByteBufRX2[0]==0xB7)&&(ucByteBufRX2[1]==0x01))
		{	
			ucByteBufRX2[0]=0;
			USART2summRXniu=crc16(ucByteBufRX2, DMARX2C);  //ÅÐ¶ÏCRC
		  if((sumc(ucByteBufRX2,DMARX2C-1))==ucByteBufRX2[DMARX2C-1])
			{
				
			 Rx_summ(Rx2Buf, ucByteBufRX2,DMARX2C-1);	//Ð£ÑéÕýÈ·¸³Öµ¸øÊý×é
			 if(USART2summRXniu!=USART2summRXold)  //ÅÐ¶ÏÀÛ¼ÓºÍÊÇ·ñÏàÍ¬£¬¿ªÆôOLEDÏÔÊ¾
			 {
				RGBWon=59;	//¿ªÆôRGBW
       }
			  USART2summRXold=USART2summRXniu;  //¸³Öµ¸ø¾ÉÖµ
			  ucByteBufRX2[DMARX2C-1]=0;
      }			 
    }		
  }   
}
//=============================================================================
//½ØÈ¡OLEDµÆ¹âÏÔÊ¾
void OLEDtinct(u8 *USART,u8 *oled,u8 addr,u8 len)//½ØÈ¡OLEDµÆ¹âÏÔÊ¾
{
	u8 i,j=0;
	
	for(i=addr;i<len+addr;i++)
	{
		oled[j]=USART[i];
		j++;
  }
}

//===========================================================================
////Ð£ÑéÈ¥³ýÖ¡Í·¼ÓµØÖ·Á½¸ö×Ö½Ú  ½ÓÊÕÐ£Ñé
u16 Rx_summ(volatile u8 *Buf1, volatile u8 *Buf2,u16 len) //ÀÛ¼ÓºÍÐ£Ñé
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
	
		return  (sumdata&0x00ff); //Ò»¸ö×Ö½ÚÀÛ¼ÓºÍ
  }
//==============================================================================
u16 sumc(volatile u8 *Buf1, u16 len) //ÀÛ¼ÓºÍÐ£Ñé
	{
		u16 sumdata=0;
		u16 i;
		
		for(i=2;i<len;i++)
		{	 
		 sumdata+=Buf1[i];	
    }		
		return  (sumdata&0x00ff); //Ò»¸ö×Ö½ÚÀÛ¼ÓºÍ
  }	
	
//========ÇåÁãÊý×é=================================================================	
	
void	memst(volatile u8 *Buf1, u16 len) //ÇåÁãÊý×é
	{
		u16 i;
		for(i=0;i<len;i++)
		{
			Buf1[i]=0;
					
    }	
  }
	
//=========================================================================		
void interrupt(u8 bit)  //¹Ø±Õ´®¿ÚÖÐ¶Ï
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
	
//===========================keytim	





void key(void)
{
	
	if(key1){ if(keytim[0]<keycon) keytim[0]++;  else   KeyBYTE[0]&=~(1<<0);} //1
      else { if(keytim[0]>keycon)keytim[0]--;  else   KeyBYTE[0]|=0x01;CANkey=1; } 
	
	if(key2){ if(keytim[1]<keycon) keytim[1]++;  else   KeyBYTE[0]&=~(1<<1);} //2
      else { if(keytim[1]>keycon)keytim[1]--;  else   KeyBYTE[0]|=0x02;CANkey=2;} 
	
	if(key3){ if(keytim[2]<keycon) keytim[2]++;  else   KeyBYTE[0]&=~(1<<2);} //3
      else { if(keytim[2]>keycon)keytim[2]--;  else   KeyBYTE[0]|=0x04;CANkey=3;} 
	
	if(key4){ if(keytim[3]<keycon) keytim[3]++;  else   KeyBYTE[0]&=~(1<<3);} //4
      else { if(keytim[3]>keycon)keytim[3]--;  else   KeyBYTE[0]|=0x08;CANkey=4;} 
	
	if(key5){ if(keytim[4]<keycon) keytim[4]++;  else   KeyBYTE[0]&=~(1<<4);} //5
      else { if(keytim[4]>keycon)keytim[4]--;  else   KeyBYTE[0]|=0x10;CANkey=5;} 
	
	if(key6){ if(keytim[5]<keycon) keytim[5]++;  else   KeyBYTE[0]&=~(1<<5);}  //6 
      else { if(keytim[5]>keycon)keytim[5]--;  else   KeyBYTE[0]|=0x20;CANkey=6;} 
	
	if(key7){ if(keytim[6]<keycon) keytim[6]++;  else   KeyBYTE[0]&=~(1<<6);} //7
      else { if(keytim[6]>keycon)keytim[6]--;  else   KeyBYTE[0]|=0x40;CANkey=7;} 		
					
	if(key8){ if(keytim[7]<keycon) keytim[7]++;  else   KeyBYTE[0]&=~(1<<7);} //8
      else { if(keytim[7]>keycon)keytim[7]--;  else   KeyBYTE[0]|=0x80;CANkey=8;} 		
			
	//===		
	if(key9){ if(keytim[8]<keycon) keytim[8]++;  else   KeyBYTE[1]&=~(1<<0);} //9
      else { if(keytim[8]>keycon)keytim[8]--;  else   KeyBYTE[1]|=0x01; CANkey=9;}		
			
	if(key10){ if(keytim[9]<keycon) keytim[9]++; else    KeyBYTE[1]&=~(1<<1);} //10
      else { if(keytim[9]>keycon)keytim[9]--;  else    KeyBYTE[1]|=0x02;CANkey=10;} 		
			
	if(key11){ if(keytim[10]<keycon) keytim[10]++; else    KeyBYTE[1]&=~(1<<2);} //3
      else { if(keytim[10]>keycon)keytim[10]--;  else    KeyBYTE[1]|=0x04;CANkey=11;} 		
		
  if(key12){ if(keytim[11]<keycon) keytim[11]++; else    KeyBYTE[1]&=~(1<<3);} //4
      else { if(keytim[11]>keycon)keytim[11]--;  else    KeyBYTE[1]|=0x08;CANkey=12;}

	if(key13){ if(keytim[12]<keycon) keytim[12]++;  else   KeyBYTE[1]&=~(1<<4);} //4
      else { if(keytim[12]>keycon)keytim[12]--;   else   KeyBYTE[1]|=0x10;CANkey=13;}		
		
  if(key14){ if(keytim[13]<keycon) keytim[13]++;  else   KeyBYTE[1]&=~(1<<5);}  //6 
      else { if(keytim[13]>keycon)keytim[13]--;   else   KeyBYTE[1]|=0x20;CANkey=14;} 
	
	if(key15){ if(keytim[14]<keycon) keytim[14]++;  else   KeyBYTE[1]&=~(1<<6);} //7
      else { if(keytim[14]>keycon)keytim[14]--;   else   KeyBYTE[1]|=0x40;CANkey=15;} 		
					
	if(key16){ if(keytim[15]<keycon) keytim[15]++;  else   KeyBYTE[1]&=~(1<<7);} //8
      else { if(keytim[15]>keycon)keytim[15]--;   else   KeyBYTE[1]|=0x80;CANkey=16;} 		
			
			
	if(key17){ if(keytim[16]<keycon) keytim[16]++;  else   KeyBYTE[2]&=~(1<<0);} //9
      else { if(keytim[16]>keycon)keytim[16]--;  else   KeyBYTE[2]|=0x01;CANkey=17; }		
			
	if(key18){ if(keytim[17]<keycon) keytim[17]++; else    KeyBYTE[2]&=~(1<<1);} //10
      else { if(keytim[17]>keycon)keytim[17]--;  else    KeyBYTE[2]|=0x02;CANkey=18;} 		
			
	if(key19){ if(keytim[18]<keycon) keytim[18]++; else    KeyBYTE[2]&=~(1<<2);} //3
      else { if(keytim[18]>keycon)keytim[18]--;  else    KeyBYTE[2]|=0x04;CANkey=19;} 		
		
  if(key20){ if(keytim[19]<keycon) keytim[19]++; else    KeyBYTE[2]&=~(1<<3);} //4
      else { if(keytim[19]>keycon)keytim[19]--;  else    KeyBYTE[2]|=0x08;CANkey=20;}

	if(key21){ if(keytim[20]<keycon) keytim[20]++;  else   KeyBYTE[2]&=~(1<<4);} //4
      else { if(keytim[20]>keycon)keytim[20]--;   else   KeyBYTE[2]|=0x10;CANkey=21;}		
		
  if(key22){ if(keytim[21]<keycon) keytim[21]++;  else   KeyBYTE[2]&=~(1<<5);}  //6 
      else { if(keytim[21]>keycon)keytim[21]--;   else   KeyBYTE[2]|=0x20;CANkey=22;} 
	
	if(key23){ if(keytim[22]<keycon) keytim[22]++;  else   KeyBYTE[2]&=~(1<<6);} //7
      else { if(keytim[22]>keycon)keytim[22]--;   else   KeyBYTE[2]|=0x40;CANkey=23;} 		
					
	if(key24){ if(keytim[23]<keycon) keytim[23]++;  else   KeyBYTE[2]&=~(1<<7);} //8
      else { if(keytim[23]>keycon)keytim[23]--;   else   KeyBYTE[2]|=0x80;CANkey=24;} 				
					
}
	
	//=======================================================





void USART3_RD(void)
{
	 //´®¿Ú3ÔÝÊ±²»ÓÃ
//   if((usart3time1>=20)||(usart3time2>=1))  //·¢ËÍÊý¾Ý
// 	{
// 		usart3time2=0; usart3time1=0; USART3_TxOV=0;		
// 		ucByteBufTX3[DMATX3C-1]=cumulative_summ(ucByteBufTX3, Tx3Buf,DMATX3C-1);	//ÀÛ¼ÓºÍ
// 		Tx3Buf[0]=TFH2; ucByteBufTX3[0]=TFH2;
// 		DMA_USART3_Tx(DMA1_Channel2,DMATX3C);		 //·¢ËÍÊý¾Ý		
//   }
// 		
// 	if(USART3_RxOV>=1)
// 	{
// 		USART3_RxOV=0;
// 	
// 	 if(ucByteBufRX3[0]==TFH2)  //ÅÐ¶ÏÖ¡Í·
// 		{		
// 		 ucByteBufRX3[0]=0;
// 		 if((cumulative_sumc(ucByteBufRX3,DMARX3C-1))==ucByteBufRX3[DMARX3C-1])
// 		 {
// 			cumulative_summ(Rx3Buf, ucByteBufRX3,DMARX3C-1);
//      }			
//     } 	
//   } 
}
//===============================================================================	
	

void  delayy(u16 i)
{
		while(i--);
}



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
/* CRCµÍÎ»×Ö½ÚÖµ±í*/
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


/******************************************************************
¹¦ÄÜ: CRC16Ð£Ñé
ÊäÈë: Ö¸Ïò´ýÐ£ÑéÊý¾ÝµÄÖ¸Õë + Ð£ÑéÊý¾Ý³¤¶È
Êä³ö: CRC¼ÆËã½á¹û
ËµÃ÷: Í¨ÓÃ³ÌÐò
******************************************************************/
unsigned int crc16(volatile unsigned char * puchMsg, unsigned int usDataLen)
{
    unsigned char uchCRCHi = 0xFF ; 						// ¸ßCRC×Ö½Ú³õÊ¼»¯
    unsigned char uchCRCLo = 0xFF ; 						// µÍCRC ×Ö½Ú³õÊ¼»¯
    unsigned long uIndex ; 							// CRCÑ­»·ÖÐµÄË÷Òý

    while (usDataLen--) 							// ´«ÊäÏûÏ¢»º³åÇø
    {
        uIndex = uchCRCHi ^ *puchMsg++ ; 			// ¼ÆËãCRC
        uchCRCHi = uchCRCLo ^ auchCRCHi[uIndex] ;
        uchCRCLo = auchCRCLo[uIndex] ;
    }

    return (uchCRCHi << 8 | uchCRCLo) ;
}



































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
	
	
	*/








