#ifndef __LED_H
#define __LED_H	 
#include "sys.h"

#define A   1

#define R485_1 PCout(9)// PE5	
#define R485_2 PAout(8)// PE5	

#define LED2 PBout(9)


void KEY_Init(void);//IO初始化
u8 KEY_Scan(u8);  	//按键扫描函数
void LED_Init(void);//初始化


//#define LED1 PAout(1)// PB5


#define key1    PEin(14) //
#define key2    PEin(12)
#define key3    PEin(10)
#define key4    PEin(7)
#define key5    PBin(0)
#define key6    PCin(4)
#define key7    PCin(3)
#define key8    PCin(1)
#define key9    PBin(7)
#define key10   PDin(7)
#define key11   PDin(6)
#define key12   PDin(4)
#define key13   PEin(13)
#define key14   PEin(11)
#define key15   PEin(9)
#define key16   PEin(8)
#define key17   PBin(1)
#define key18   PCin(5)
#define key19   PAin(1)
#define key20   PCin(2)
#define key21   PBin(8)
#define key22   PBin(6)
#define key23   PDin(5)
#define key24   PDin(3)


#define k1   PDin(11)
#define k2   PDin(12)
#define k3   PDin(13)
#define k4   PDin(14)



#endif
