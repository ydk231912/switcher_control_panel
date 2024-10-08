#ifndef __LED_H
#define __LED_H	 
#include "sys.h"

#define A   0

#define R485_1 	PDout(5)// PE5	

#define LED2 	PDout(14)// PE5	


void KEY_Init(void);//IO初始化
u8 KEY_Scan(u8);  	//按键扫描函数
void LED_Init(void);//初始化




#define key1    PCin(4) 
#define key2    PBin(1)
#define key3    PEin(9)
#define key4    PCin(5)
#define key5    PEin(7)
#define key6    PEin(10)
#define key7    PBin(0)
#define key8    PEin(8)
#define key9    PEin(11)
#define key10   PEin(0)
#define key11   PBin(7)
#define key12   PDin(6)
#define key13   PEin(1)
#define key14   PBin(8)
#define key15   PDin(7)
#define key16   PBin(9)
#define key17   PBin(6)
#define key18   PEin(12)
#define key19   PEin(13)
#define key20   PEin(15)
#define key21   PEin(14)
#define key22   PDin(11)
#define key23   PDin(9)
#define key24   PDin(12)
#define key25   PDin(8)






















#endif
