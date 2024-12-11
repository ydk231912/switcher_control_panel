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


u8 Tx1Buf[DMATX1];  //´®¿ÚÍ¨Ñ¶
u8 Rx1Buf[DMARX1];
u8 Tx2Buf[DMATX2];
u8 Rx2Buf[DMARX2];
u8 Tx3Buf[DMATX3];
u8 Rx3Buf[DMARX3];

signed char keytim[25]= {0}; //°´¼üÑÓÊ±
u8  candata[8];
u8  KeyBYTE[8]= {0};
u16 temp1,temp5;
u8  USART2summRXold=0,USART2summRXniu;
u8  CANkey=0;           //CAN¿ª¹Ø
u8  CAN1_SendC=0;      //·¢ËÍÊ§°Ü¼ÆÊý
u8  CAN1_keycount=0;   //°´¼ü·¢ËÍ³É¹¦ºóÑÓÊ±¶Ï¿ª
u8  CAN1_sw=0;         //can¿ª¹Ø
u8  OLEDold=0;         //·À¶¶¼ÆÊý
//u16  oleddatacnt=0;
u8  RGBWDelay=0;       //RGBWÑÓÊ±¶Ï¿ª
u8  RGBWon=0;
u8  oleddata;   //	OLEDÏÔÊ¾½ø¶ÈÌõ
u8  CAN1times=0;  //CAN·¢ËÍÊ±¼ä
u16 ADCtemp=0;   //ADÖÐ¼äÊý¾Ý


void fun(void)
{
    if((rgbwtim>=10)&&(1))  //·¢ËÍRGBWµÆ¹â
    {
        OLEDtinct(Rx2Buf,rgbw2814,0,25);  //½ØÈ¡OLEDÏÔÊ¾Êý¾Ý
        WS2812B_Task();  //RGBW°´¼ü CANaddr
        rgbwtim=0;
        if(RGBWon==37)
        {
            //WS2812B_Task();  //RGBW°´¼ü CANaddr
            RGBWDelay++;
            if(RGBWDelay>=2)  //·¢ËÍ3´Îºó¶Ï¿ªRGBWÊý¾Ý
            {
                RGBWon=0;
                rgbwtim=0;
            }
        }
    }
//====================================================================
    key();  //°´¼ü
    IWDG_Feed();//=========================
    candata[0]=KeyBYTE[0];
    candata[1]=KeyBYTE[1];
    candata[2]=KeyBYTE[2];
    candata[3]=KeyBYTE[3];

//============================================================================
    if(cantim>200)cantim=210;
    if(CAN1_keycount>100)	CAN1_keycount=0;

#if U1orU2

    CAN1times=2;   //2U¶¨Ê±·¢ËÍÊ±¼ä

    if(((CANkey>0)&&(CANkey<=25)))  //°´¼ü·¢ËÍCANÃüÁî
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
    CAN1_sw=0;     //È¡Ïû°´¼ü°´ÏÂ·¢ËÍ£¬¸ÄÎª¶¨Ê±·¢ËÍ
    CAN1times=3;	 //1U¶¨Ê±·¢ËÍÊ±¼ä
#endif

//======================================================================
    //°´¼ü°´ÏÂÊ±£¬Ò²ÒªµÈ´ý2ºÁÃë×óÓÒ·¢ËÍÊý¾Ý£¬·ÀÖ¹ÆäËû´ÓÕ¾Êý¾ÝÓµ¶Â¡£Èç²»ÐèÒª¿ÉÐ´1
    if(((CAN1_sw)&&(cantim>=1))||(cantim>=CAN1times))   //°´¼üºÍ¶¨Ê±×Ô¶¯·¢ËÍ
    {
        CAN1_keycount++;                  //°´¼üÊÍ·Åºó·¢ËÍ¼ÆÊý
        cantim=0;
        if(CAN1_Send_Num(0x602,candata))  //ÅÐ¶Ï·¢ËÍÊÇ·ñÕýÈ·
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

    IWDG_Feed();
//==================================================================
//´®¿ÚÍ¨ÐÅ
    USART2_RD();
    CAN1_err();   //CAN´íÎó¼ì²â
//==================================================================
    Get_Adc_Average1(10); //Ä£ÄâÁ¿²É¼¯ÂË²¨

    if(1)  //OLED½ø¶ÈÌõÏÔÊ¾
    {
        if(ADtim>2)ADtim=31;
        if(ADtim>=2)
        {
            if(4000>=ADCsim1)	{
                ADCtemp=4000-ADCsim1;
            }

            candata[5]=ADCtemp;
            candata[4]=ADCtemp>>8;  //CAN·¢ËÍÊý¾Ý
            oleddata=(ADCsim1/153);//ÍÆ×ÓÄ£ÄâÁ¿

        }
        else
        {
            oleddata=0;
        }
    }
    else
        oleddata=cantxerr;//²âÊÔ£¬ÏÔÊ¾CAN´íÎó±ê¼Ç


    if(OLEDtim>10)  // ÖÜÆÚÏÔÊ¾OLED½ø¶ÈÌõ
    {
        OLEDtim=0;
        if(oleddata>26)oleddata=26;
        OLED_ShowString(8,18,oleddata,64,0);  //OLEDÆÁÏÔÊ¾½ø¶ÈÌõ
    }

//==================================================================
//Íâ²¿Ö¸Ê¾µÆ
    if(temp5>=200)temp5=0;
    temp1++;
    if(temp1>=1000) {
        LED2=!LED2;
        temp1=0;
        temp5++;
    }

}
//==============================================================================
//=============================================================================
//½ØÈ¡OLEDµÆ¹âÏÔÊ¾
void OLEDtinct(u8 *USART,u8 *oled,u8 addr,u8 len)//½ØÈ¡OLEDµÆ¹âÏÔÊ¾
{
    u8 i,j=0;

    for(i=addr; i<len+addr; i++)
    {
        oled[j]=USART[i];
        j++;
    }
}

//==================ÓëRAMÖ÷¿Ø°åÍ¨Ñ¶=============================================================
void USART1_RD(void)
{

    if(USART1_RxOV>=1)
    {
        USART1_RxOV=0;
        if((ucByteBufRX1[0]==0xA8)&&(ucByteBufRX1[1]==0x01))  //ÅÐ¶ÏÊÇ·ñÊÇÐ´OLEDÃüÁî
        {
            ucByteBufRX1[0]=0;
            ucByteBufRX1[1]=0;
            if((sumc(ucByteBufRX1,DMARX1C-1))==ucByteBufRX1[DMARX1C-1])
            {
                //if(ucByteBufRX1[1]==0x01)  //ÅÐ¶ÏµØÖ·ÊÇÏÔÊ¾ÆÁ»¹ÊÇÊý¾Ý
                {
                    Rx_summ(Tx3Buf, ucByteBufRX1,DMARX1C-1);//´®¿Ú1½ÓÊÕµ½Êý¾ÝÐ´Èëµ½´®¿Ú3·¢ËÍ
                }
            }
        }
    }
}
//===================·¢ËÍ°´¼üµÆ¹âÃüÁî===============================================
void USART2_RD(void)
{

    //´®¿Ú1½ÓÊÕOLEDÊý¾Ý£¬ÍÆ×Ó°åÃ»ÓÐOLEDÆÁÊý¾Ý²»´¦Àí
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
                    RGBWon=37;	//¿ªÆôRGBW

                }
                USART2summRXold=USART2summRXniu;  //¸³Öµ¸ø¾ÉÖµ
                ucByteBufRX2[DMARX2C-1]=0;
            }
        }
    }
}
//====================·¢ËÍOLEDÏÔÊ¾Êý¾ÝÃüÁî====================================
//485´®¿Ú·¢ËÍÊý¾Ýµ½´Ó»ú£¬·¢ËÍOLEDÏÔÊ¾ÆÁÊý¾Ý

void USART3_RD(void)
{
    ;
}


//=================·¢ËÍÀÛ¼Ó½»»»Ð£ÑéºÍ==========================================================
//cumulative_summ(ucByteBufTX2, Tx2Buf,DMATX2C-1)
//Ð£ÑéÈ¥³ýÖ¡Í·¼ÓµØÖ·Á½¸ö×Ö½Ú ·¢ËÍÐ£Ñé
u16 Tx_summ(volatile u8 *Buf1, volatile u8 *Buf2,u16 len) //ÀÛ¼ÓºÍÐ£Ñé
{
    u16 sumdata=0;
    u16 i;
    u16 j=0;
    for(i=2; i<len; i++)
    {
        Buf1[j+2]=Buf2[j];	//·¢ËÍ×Ö·û´®
        sumdata+=Buf2[j];
        j++;
    }
    return  (sumdata&0x00ff); //Ò»¸ö×Ö½ÚÀÛ¼ÓºÍ
}
//===========½ÓÊÕÐ£Ñé===========================================================
////Ð£ÑéÈ¥³ýÖ¡Í·¼ÓµØÖ·Á½¸ö×Ö½Ú  ½ÓÊÕÐ£Ñé
u16 Rx_summ(volatile u8 *Buf1, volatile u8 *Buf2,u16 len) //ÀÛ¼ÓºÍÐ£Ñé
{
    u16 sumdata=0;
    u16 i;
    u16 j=0;

    for(i=1; i<len; i++)
    {
        Buf1[j]=Buf2[j+2];
        sumdata+=Buf2[j+2];
        j++;
    }

    return  (sumdata&0x00ff); //Ò»¸ö×Ö½ÚÀÛ¼ÓºÍ
}

//==============ÀÛ¼ÓÐ£ÑéºÍ================================================================
u16 sumc(volatile u8 *Buf1, u16 len) //ÀÛ¼ÓºÍÐ£Ñé
{
    u16 sumdata=0;
    u16 i;

    for(i=2; i<len; i++)
    {
        sumdata+=Buf1[i];
    }
    return  (sumdata&0x00ff); //Ò»¸ö×Ö½ÚÀÛ¼ÓºÍ
}

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


void key(void)
{

    if(key1) {
        if(keytim[0]<keycon) keytim[0]++;     //1
        else   KeyBYTE[0]&=~(1<<0);
    }
    else {
        if(keytim[0]>keycon)keytim[0]--;
        else   KeyBYTE[0]|=0x01;
        CANkey=1;
    }

    if(key2) {
        if(keytim[1]<keycon) keytim[1]++;     //2
        else   KeyBYTE[0]&=~(1<<1);
    }
    else {
        if(keytim[1]>keycon)keytim[1]--;
        else   KeyBYTE[0]|=0x02;
        CANkey=2;
    }

    if(key3) {
        if(keytim[2]<keycon) keytim[2]++;     //3
        else   KeyBYTE[0]&=~(1<<2);
    }
    else {
        if(keytim[2]>keycon)keytim[2]--;
        else   KeyBYTE[0]|=0x04;
        CANkey=3;
    }

    if(key4) {
        if(keytim[3]<keycon) keytim[3]++;     //4
        else   KeyBYTE[0]&=~(1<<3);
    }
    else {
        if(keytim[3]>keycon)keytim[3]--;
        else   KeyBYTE[0]|=0x08;
        CANkey=4;
    }

    if(key5) {
        if(keytim[4]<keycon) keytim[4]++;     //5
        else   KeyBYTE[0]&=~(1<<4);
    }
    else {
        if(keytim[4]>keycon)keytim[4]--;
        else   KeyBYTE[0]|=0x10;
        CANkey=5;
    }

    if(key6) {
        if(keytim[5]<keycon) keytim[5]++;     //6
        else   KeyBYTE[0]&=~(1<<5);
    }
    else {
        if(keytim[5]>keycon)keytim[5]--;
        else   KeyBYTE[0]|=0x20;
        CANkey=6;
    }

    if(key7) {
        if(keytim[6]<keycon) keytim[6]++;     //7
        else   KeyBYTE[0]&=~(1<<6);
    }
    else {
        if(keytim[6]>keycon)keytim[6]--;
        else   KeyBYTE[0]|=0x40;
        CANkey=7;
    }

    if(key8) {
        if(keytim[7]<keycon) keytim[7]++;     //8
        else   KeyBYTE[0]&=~(1<<7);
    }
    else {
        if(keytim[7]>keycon)keytim[7]--;
        else   KeyBYTE[0]|=0x80;
        CANkey=8;
    }

    //===
    if(key9) {
        if(keytim[8]<keycon) keytim[8]++;     //9
        else   KeyBYTE[1]&=~(1<<0);
    }
    else {
        if(keytim[8]>keycon)keytim[8]--;
        else   KeyBYTE[1]|=0x01;
        CANkey=9;
    }

    if(key10) {
        if(keytim[9]<keycon) keytim[9]++;    //10
        else    KeyBYTE[1]&=~(1<<1);
    }
    else {
        if(keytim[9]>keycon)keytim[9]--;
        else    KeyBYTE[1]|=0x02;
        CANkey=10;
    }

    if(key11) {
        if(keytim[10]<keycon) keytim[10]++;    //3
        else    KeyBYTE[1]&=~(1<<2);
    }
    else {
        if(keytim[10]>keycon)keytim[10]--;
        else    KeyBYTE[1]|=0x04;
        CANkey=11;
    }

    if(key12) {
        if(keytim[11]<keycon) keytim[11]++;    //4
        else    KeyBYTE[1]&=~(1<<3);
    }
    else {
        if(keytim[11]>keycon)keytim[11]--;
        else    KeyBYTE[1]|=0x08;
        CANkey=12;
    }

    if(key13) {
        if(keytim[12]<keycon) keytim[12]++;     //4
        else   KeyBYTE[1]&=~(1<<4);
    }
    else {
        if(keytim[12]>keycon)keytim[12]--;
        else   KeyBYTE[1]|=0x10;
        CANkey=13;
    }

    if(key14) {
        if(keytim[13]<keycon) keytim[13]++;     //6
        else   KeyBYTE[1]&=~(1<<5);
    }
    else {
        if(keytim[13]>keycon)keytim[13]--;
        else   KeyBYTE[1]|=0x20;
        CANkey=14;
    }

    if(key15) {
        if(keytim[14]<keycon) keytim[14]++;     //7
        else   KeyBYTE[1]&=~(1<<6);
    }
    else {
        if(keytim[14]>keycon)keytim[14]--;
        else   KeyBYTE[1]|=0x40;
        CANkey=15;
    }

    if(key16) {
        if(keytim[15]<keycon) keytim[15]++;     //8
        else   KeyBYTE[1]&=~(1<<7);
    }
    else {
        if(keytim[15]>keycon)keytim[15]--;
        else   KeyBYTE[1]|=0x80;
        CANkey=16;
    }


    if(key17) {
        if(keytim[16]<keycon) keytim[16]++;     //9
        else   KeyBYTE[2]&=~(1<<0);
    }
    else {
        if(keytim[16]>keycon)keytim[16]--;
        else   KeyBYTE[2]|=0x01;
        CANkey=17;
    }

    if(key18) {
        if(keytim[17]<keycon) keytim[17]++;    //10
        else    KeyBYTE[2]&=~(1<<1);
    }
    else {
        if(keytim[17]>keycon)keytim[17]--;
        else    KeyBYTE[2]|=0x02;
        CANkey=18;
    }

    if(key19) {
        if(keytim[18]<keycon) keytim[18]++;    //3
        else    KeyBYTE[2]&=~(1<<2);
    }
    else {
        if(keytim[18]>keycon)keytim[18]--;
        else    KeyBYTE[2]|=0x04;
        CANkey=19;
    }

    if(key20) {
        if(keytim[19]<keycon) keytim[19]++;    //4
        else    KeyBYTE[2]&=~(1<<3);
    }
    else {
        if(keytim[19]>keycon)keytim[19]--;
        else    KeyBYTE[2]|=0x08;
        CANkey=20;
    }

    if(key21) {
        if(keytim[20]<keycon) keytim[20]++;     //4
        else   KeyBYTE[2]&=~(1<<4);
    }
    else {
        if(keytim[20]>keycon)keytim[20]--;
        else   KeyBYTE[2]|=0x10;
        CANkey=21;
    }

    if(key22) {
        if(keytim[21]<keycon) keytim[21]++;     //6
        else   KeyBYTE[2]&=~(1<<5);
    }
    else {
        if(keytim[21]>keycon)keytim[21]--;
        else   KeyBYTE[2]|=0x20;
        CANkey=22;
    }

    if(key23) {
        if(keytim[22]<keycon) keytim[22]++;     //7
        else   KeyBYTE[2]&=~(1<<6);
    }
    else {
        if(keytim[22]>keycon)keytim[22]--;
        else   KeyBYTE[2]|=0x40;
        CANkey=23;
    }

    if(key24) {
        if(keytim[23]<keycon) keytim[23]++;     //8
        else   KeyBYTE[2]&=~(1<<7);
    }
    else {
        if(keytim[23]>keycon)keytim[23]--;
        else   KeyBYTE[2]|=0x80;
        CANkey=24;
    }

    if(key25) {
        if(keytim[24]<keycon) keytim[24]++;     //8
        else   KeyBYTE[3]&=~(1<<0);
    }
    else {
        if(keytim[24]>keycon)keytim[24]--;
        else   KeyBYTE[3]|=0x01;
        CANkey=25;
    }
}








//=====================================================================



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







	u16 cumulative_summ(volatile u8 *Buf1, volatile u8 *Buf2,u16 len) //ÀÛ¼ÓºÍÐ£Ñé
	{
		u16 sumdata=0;
		u16 i;

		for(i=1;i<len;i++)
		{
		 *(Buf1+i)=*(Buf2+i);
		 sumdata+=*(Buf2+i);
    }
		return  (sumdata&0x00ff); //Ò»¸ö×Ö½ÚÀÛ¼ÓºÍ
  }








	*/








