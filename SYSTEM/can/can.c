#include "can.h"

//===================================================================================
/************************************************************
函数功能：CAN1初始化
入口参数：tsjw:重新同步跳跃时间单元，范围1~3
          tbs2：时间段2的时间单元，范围1~8
		  tbs1：时间段1的时间单元，范围1~16
		  brp：波特率分配器，范围：1~1024；（实际要加1，也就是1~1024）tq=（brp）*tpclk1
		  mode：0，普通模式，1回环模式
返回值：0初始化成功，其他均为初始化失败
注意：除了mode可以为0，其余均不可以；
波特率=Fpclk/（（tbs1+tbs2+1）*brp），Fpclk为36MHz
      =36/（（3+2+1）*6）=1MHz
*************************************************************/
u8 CAN1_Mode_Init(u8 tsjw,u8 tbs2,u8 tbs1,u16 brp,u8 mode)
{
	GPIO_InitTypeDef GPIO_InitStructure;
//	NVIC_InitTypeDef NVIC_InitStructure;
	CAN_FilterInitTypeDef CAN_FilterInitStructure;
	u16 i=0;
 	if(tsjw==0||tbs2==0||tbs1==0||brp==0)return 1;
	tsjw-=1; 				//先减去1.再用于设置
	tbs2-=1;
	tbs1-=1;
	brp-=1;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode= GPIO_Mode_IPU;
	GPIO_Init(GPIOA,&GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Mode= GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&GPIO_InitStructure);
	
	CAN1->MCR=0X0000;		//退出睡眠模式
	CAN1->MCR|=1<<0;		//请求can1进入初始化模式
	while((CAN1->MCR&1<<0)==0)
	{
		i++;
		if(i>100) return 2;	//进入初始化模式失败
	}
	CAN1->MCR|=0<<7;		//非时间触发通信模式
	CAN1->MCR|=0<<6;		//软件自动离线管理
	CAN1->MCR|=1<<4;		//进制报文自动发送
	CAN1->MCR|=0<<3;		//报文不锁定，新的覆盖就的
	CAN1->MCR|=0<<2;		//优先级由报文标识符决定
	CAN1->BTR =0x00000000;	//清除原来的设置
	CAN1->BTR|=mode<<30;	//设置模式，0为普通模式，1为回环模式
	CAN1->BTR|=tsjw<<24;	//重新同步跳跃宽度为tsjw+1个单位时间
	CAN1->BTR|=tbs2<<20;	//tbs2=tbs2+1个时间单位
	CAN1->BTR|=tbs1<<16;	//tsb1=tsb1+1个时间单位
	CAN1->BTR|=brp<<0;		//分频系数为brp+1,波特率为36MHz/（tbs1+tbs2+1）/fdiv
	CAN1->MCR&=~(1<<0);
	while((CAN1->MCR&1<<0)==1)
	{
		i++;
		if(i>0XFFF0) return 3;//退出初始化模式失败
	}
	CAN_FilterInitStructure.CAN_FilterNumber = 1;						//指定过滤器为1
	CAN_FilterInitStructure.CAN_FilterMode   = CAN_FilterMode_IdMask;	//指定过滤器为标识符屏蔽位模式
	CAN_FilterInitStructure.CAN_FilterScale  = CAN_FilterScale_32bit;	//过滤器位宽为32位
	CAN_FilterInitStructure.CAN_FilterIdHigh = 0x0000;					//过滤器标识符的高十六位
	CAN_FilterInitStructure.CAN_FilterMaskIdLow=0x0000;					//过滤器标识符的低十六位
	CAN_FilterInitStructure.CAN_FilterMaskIdHigh=0x0000;				//过滤器屏蔽标识符的高十六位
	CAN_FilterInitStructure.CAN_FilterMaskIdLow =0x0000;				//过滤器屏蔽标识符的低十六位
	CAN_FilterInitStructure.CAN_FilterFIFOAssignment=CAN_FIFO0;			//指定过滤的FIFO为0
	CAN_FilterInitStructure.CAN_FilterActivation=ENABLE;				//使能过滤器
	CAN_FilterInit(&CAN_FilterInitStructure);
	
#if CAN1_RX0_INT_ENABLE
	NVIC_InitStructure.NVIC_IRQChannel = USB_LP_CAN1_RX0_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	CAN_ITConfig(CAN1,CAN_IT_FMP0,ENABLE);
#endif
	return 0;
}

//===================================================================================
/**************************************************************************
函数功能：CAN发送数据
入口参数：id:标准ID(11位)/扩展ID(11位+18位)	    
			    ide:0,标准帧;1,扩展帧
			    rtr:0,数据帧;1,远程帧
			    len:要发送的数据长度(固定为8个字节,在时间触发模式下,有效数据为6个字节)
			    *dat:数据指针.
返回  值：0~3,邮箱编号.0XFF,无有效邮箱
**************************************************************************/
u8 CAN1_Tx_Msg(u32 id,u8 ide,u8 rtr,u8 len,u8 *dat)
{
	u8 mbox;
	if(CAN1->TSR&(1<<26)) mbox=0;		//邮箱o为空
	else if(CAN1->TSR&(1<<27)) mbox=1;	//邮箱1为空
	else if(CAN1->TSR&(1<<28)) mbox=2;	//邮箱2为空
	else return 0XFF;					//无空邮箱，无法发送
	
	CAN1->sTxMailBox[mbox].TIR=0;		//清除之前的设置
	if(ide==0)							//标准帧
	{
		id&=0x7ff;						//取得低11位的stdid
		id<<=21;
	}
	else								//扩展帧
	{
		id&=0X1FFFFFFF;					//取得低32位的extid
		id<<=3;
	}
	CAN1->sTxMailBox[mbox].TIR|=id;
	CAN1->sTxMailBox[mbox].TIR|=ide<<2;
	CAN1->sTxMailBox[mbox].TIR|=rtr<<1;
	len&=0X0F;							//得到低四位
	CAN1->sTxMailBox[mbox].TDTR&=~(0X0000000F);
	CAN1->sTxMailBox[mbox].TDTR|=len;	//设置DLC
	//待发送数据存入邮箱
	CAN1->sTxMailBox[mbox].TDHR=(((u32)dat[7]<<24)|
								((u32)dat[6]<<16)|
 								((u32)dat[5]<<8)|
								((u32)dat[4]));
	CAN1->sTxMailBox[mbox].TDLR=(((u32)dat[3]<<24)|
								((u32)dat[2]<<16)|
 								((u32)dat[1]<<8)|
								((u32)dat[0]));
	CAN1->sTxMailBox[mbox].TIR|=1<<0; 	//请求发送邮箱数据
	return mbox;
}

//===================================================================================
/**************************************************************************
函数功能：获得发送状态
入口参数：mbox：邮箱编号
返回  值：0,挂起;0X05,发送失败;0X07,发送成功
**************************************************************************/
u8 CAN1_Tx_Staus(u8 mbox)
{	
	u8 sta=0;					    
	switch (mbox)
	{
		case 0: 
			sta |= CAN1->TSR&(1<<0);			//RQCP0
			sta |= CAN1->TSR&(1<<1);			//TXOK0
			sta |=((CAN1->TSR&(1<<26))>>24); 	//TME0
			break;
		case 1: 
			sta |= CAN1->TSR&(1<<8)>>8;		   	//RQCP1
			sta |= CAN1->TSR&(1<<9)>>8;		   	//TXOK1
			sta |=((CAN1->TSR&(1<<27))>>25); 	//TME1	   
			break;
		case 2: 
			sta |= CAN1->TSR&(1<<16)>>16;	   	//RQCP2
			sta |= CAN1->TSR&(1<<17)>>16;	   	//TXOK2
			sta |=((CAN1->TSR&(1<<28))>>26); 	//TME2
			break;
		default:
			sta=0X05;  							//邮箱号不对,失败
		break;
	}
	return sta;
}

//===================================================================================
/**************************************************************************
函数功能：得到在FIFO0/FIFO1中接收到的报文个数
入口参数：fifox：FIFO编号（0、1）
返回  值：FIFO0/FIFO1中的报文个数
**************************************************************************/
u8 CAN1_Msg_Pend(u8 fifox)
{
	if(fifox==0)return CAN1->RF0R&0x03; 
	else if(fifox==1)return CAN1->RF1R&0x03; 
	else return 0;
}

//===================================================================================
/**************************************************************************
函数功能：接收数据
入口参数：fifox：邮箱号
		    	id:标准ID(11位)/扩展ID(11位+18位)
			    ide:0,标准帧;1,扩展帧
			    rtr:0,数据帧;1,远程帧
			    len:接收到的数据长度(固定为8个字节,在时间触发模式下,有效数据为6个字节)
			    dat:数据缓存区
返回  值：无 
**************************************************************************/
void CAN1_Rx_Msg(u8 fifox,u32 *id,u8 *ide,u8 *rtr,u8 *len,u8 *dat)
{	   
	*ide=CAN1->sFIFOMailBox[fifox].RIR&0x04;  				//得到标识符选择位的值  
 	if(*ide==0) 				 							//标准标识符
	{
		*id=CAN1->sFIFOMailBox[fifox].RIR>>21;
	}else	      											//扩展标识符
	{
		*id=CAN1->sFIFOMailBox[fifox].RIR>>3;
	}
	*rtr=CAN1->sFIFOMailBox[fifox].RIR&0x02;	 			//得到远程发送请求值
	*len=CAN1->sFIFOMailBox[fifox].RDTR&0x0F;  				//得到DLC
 	//*fmi=(CAN1->sFIFOMailBox[FIFONumber].RDTR>>8)&0xFF;	//得到FMI
	//接收数据
	dat[0]=CAN1->sFIFOMailBox[fifox].RDLR&0XFF;
	dat[1]=(CAN1->sFIFOMailBox[fifox].RDLR>>8)&0XFF;
	dat[2]=(CAN1->sFIFOMailBox[fifox].RDLR>>16)&0XFF;
	dat[3]=(CAN1->sFIFOMailBox[fifox].RDLR>>24)&0XFF;    
	dat[4]=CAN1->sFIFOMailBox[fifox].RDHR&0XFF;
	dat[5]=(CAN1->sFIFOMailBox[fifox].RDHR>>8)&0XFF;
	dat[6]=(CAN1->sFIFOMailBox[fifox].RDHR>>16)&0XFF;
	dat[7]=(CAN1->sFIFOMailBox[fifox].RDHR>>24)&0XFF;    
	if(fifox==0)CAN1->RF0R|=0X20;       					//释放FIFO0邮箱
	else if(fifox==1)CAN1->RF1R|=0X20; 						//释放FIFO1邮箱	 
}

//===================================================================================
/**************************************************************************
函数功能：CAN接收中断服务函数，条件编译
入口参数：无
返回  值：无 
**************************************************************************/
#if CAN1_RX0_INT_ENABLE	//Enable RX0 interrupt //使能RX0中断	    
//void CAN_RX_IRQHandler(void)
//{
////  u8 i;
//	u32 id;
//	u8 ide,rtr,len;     
////	u8 ON_rxbuf[8]={10,12,15,19,24,30,37,0};校验数据

//	u8 temp_rxbuf[8];
//	//Verify the first set of data, and CAN control mode will be started if the verification is successful
//	//校验第一组数据，校验成功则开启CAN控制模式
// 	CAN1_Rx_Msg(0,&id,&ide,&rtr,&len,temp_rxbuf);
//	if(id == Ctrl_ID)
//	{
//		 Ctrl_Mode      = temp_rxbuf[0];//控制模式
//		 DIR_Receive    = temp_rxbuf[1];//接收到步进电机的转向
//		 Microstep      = temp_rxbuf[2];//接收到步进电机的细分值
//		 switch(Ctrl_Mode)
//		 { 
//			 case Speed_Mode:
//				 SPEED =(float)SPeed_transition(temp_rxbuf[5],temp_rxbuf[6])/6.28318531;;//接受到步进电机的速度，多少圈/s
//				 break;
//			 case Position_Mode:
//				 if(DIR_Receive==0)
//				    Loop_T_Position =Loop_T_Position+(int)(17.77778*Microstep/32*Position_transition(temp_rxbuf[3],temp_rxbuf[4])/10.0);
//				 else
//					Loop_T_Position =Loop_T_Position-(int)(17.77778*Microstep/32*Position_transition(temp_rxbuf[3],temp_rxbuf[4])/10.0);
//			     SPEED =(float)SPeed_transition(temp_rxbuf[5],temp_rxbuf[6])/6.28318531;;//接受到步进电机的速度，多少圈/s
//				 break;
//			 case Torque_Mode:
//				 Current_value = temp_rxbuf[3];
//			     SPEED =(float)SPeed_transition(temp_rxbuf[5],temp_rxbuf[6])/6.28318531;;//接受到步进电机的速度，多少圈/s
//				break;
//			 case Absolute_Angle_Mode:
//				 Absolute_T_Position = Angle_transition(temp_rxbuf[3],temp_rxbuf[4]);
//			     SPEED =(float)SPeed_transition(temp_rxbuf[5],temp_rxbuf[6])/6.28318531;;//接受到步进电机的速度，多少圈/s
//				 break;
////			 case Multi_lap_Mode:
////				 Step_angle = temp_rxbuf[3];
////				  break;
//			 default:
//				 break;
//		 }
//	}
//	
//}
#endif

//===================================================================================
/**************************************************************************
函数功能：CAN1发送一组数据(固定格式:ID为0X601,标准帧,数据帧)
入口参数：msg:数据指针
    			len:数据长度(最大为8)
返回  值：0,成功，其他,失败;
**************************************************************************/
u8 CAN1_Send_Msg(u8* msg,u8 len)
{	
	u8 mbox;
	u16 i=0;	  	 						       
    mbox=CAN1_Tx_Msg(0X601,0,0,len,msg);   
	while((CAN1_Tx_Staus(mbox)!=0X07)&&(i<0XFFF))i++;	//等待发送结束
	if(i>=0XFFF)return 1;								//发送失败
	return 0;											//发送成功									
}

//===================================================================================
/**************************************************************************
函数功能：CAN1口接收数据查询
入口参数：buf:数据缓存区
返回  值：0,无数据被收到，其他,接收的数据长度
**************************************************************************/
u8 CAN1_Receive_Msg(u8 *buf)
{		   		   
	u32 id;
	u8 ide,rtr,len; 
	if(CAN1_Msg_Pend(0)==0)return 0;					//没有接收到数据,直接退出 	 
  	CAN1_Rx_Msg(0,&id,&ide,&rtr,&len,buf); 				//读取数据
    if(id!=0x12||ide!=0||rtr!=0)len=0;					//接收错误	   
	return len;	
}

//===================================================================================
/**************************************************************************
函数功能：CAN1发送一组数据测试
入口参数：msg:数据指针
			    len:数据长度(最大为8)
返回  值：0,成功，1,失败
**************************************************************************/
u8 CAN1_Send_MsgTEST(u8* msg,u8 len)
{	
	u8 mbox;
	u16 i=0;	  	 						       
    mbox=CAN1_Tx_Msg(0X701,0,0,len,msg);   
	while((CAN1_Tx_Staus(mbox)!=0X07)&&(i<0XFFF))i++;	//等待发送结束
	if(i>=0XFFF)return 1;								//发送失败
	return 0;											//发送成功
}

//===================================================================================
/**************************************************************************
函数功能：给给定的id发送一个数组的命令
入口参数：id：ID号
			    msg：被输送数据指针
返回  值：0,成功，1,失败
**************************************************************************/
u8 CAN1_Send_Num(u32 id,u8* msg)
{
	u8 mbox;
	u16 i=0;	  	 						       
	mbox=CAN1_Tx_Msg(id,0,0,8,msg);   
	while((CAN1_Tx_Staus(mbox)!=0X07)&&(i<0XFFF))i++;	//等待发送结束
	if(i>=0XFFF)return 1;								//发送失败
	return 0;
}



//===================================================================================
/*
#include "can.h"
#include "led.h"
#include "delay.h"
//#include "usart.h"	

//??:??????????	 
 
//===================================================================================
//CAN???
//tsjw:??????????.??:CAN_SJW_1tq~ CAN_SJW_4tq
//tbs2:???2?????.   ??:CAN_BS2_1tq~CAN_BS2_8tq;
//tbs1:???1?????.   ??:CAN_BS1_1tq ~CAN_BS1_16tq
//brp :??????.??:1~1024;  tq=(brp)*tpclk1
//???=Fpclk1/((tbs1+1+tbs2+1+1)*brp);
//mode:CAN_Mode_Normal,????;CAN_Mode_LoopBack,????;
//Fpclk1?????????????36M,????CAN_Mode_Init(CAN_SJW_1tq,CAN_BS2_8tq,CAN_BS1_9tq,4,CAN_Mode_LoopBack);
//?????:36M/((8+9+1)*4)=500Kbps
//???:0,???OK;
//    ??,?????; 
u8 CAN_Mode_Init(u8 tsjw,u8 tbs2,u8 tbs1,u16 brp,u8 mode)
{ 
	GPIO_InitTypeDef 		GPIO_InitStructure; 
	CAN_InitTypeDef        	CAN_InitStructure;
	CAN_FilterInitTypeDef  	CAN_FilterInitStructure;
#if CAN_RX0_INT_ENABLE 
	NVIC_InitTypeDef  		NVIC_InitStructure;
#endif

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);//??PORTA??	                   											 

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);//??CAN1??	

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//????
	GPIO_Init(GPIOA, &GPIO_InitStructure);			//???IO

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;	//????
	GPIO_Init(GPIOA, &GPIO_InitStructure);			//???IO

	//CAN????
	CAN_InitStructure.CAN_TTCM=DISABLE;			//?????????  
	CAN_InitStructure.CAN_ABOM=DISABLE;			//????????	 
	CAN_InitStructure.CAN_AWUM=DISABLE;			//??????????(??CAN->MCR?SLEEP?)
	CAN_InitStructure.CAN_NART=ENABLE;			//???????? 
	CAN_InitStructure.CAN_RFLM=DISABLE;		 	//?????,??????  
	CAN_InitStructure.CAN_TXFP=DISABLE;			//??????????? 
	CAN_InitStructure.CAN_Mode= mode;	        //????: mode:0,????;1,????; 
	//?????
	CAN_InitStructure.CAN_SJW=tsjw;				//????????(Tsjw)?tsjw+1?????  CAN_SJW_1tq	 CAN_SJW_2tq CAN_SJW_3tq CAN_SJW_4tq
	CAN_InitStructure.CAN_BS1=tbs1; 			//Tbs1=tbs1+1?????CAN_BS1_1tq ~CAN_BS1_16tq
	CAN_InitStructure.CAN_BS2=tbs2;				//Tbs2=tbs2+1?????CAN_BS2_1tq ~	CAN_BS2_8tq
	CAN_InitStructure.CAN_Prescaler=brp;        //????(Fdiv)?brp+1	
	CAN_Init(CAN1, &CAN_InitStructure);        	//???CAN1 

	CAN_FilterInitStructure.CAN_FilterNumber=0;	//???0
	CAN_FilterInitStructure.CAN_FilterMode=CAN_FilterMode_IdMask; 	//?????
	CAN_FilterInitStructure.CAN_FilterScale=CAN_FilterScale_32bit; 	//32?? 
	CAN_FilterInitStructure.CAN_FilterIdHigh=0x0000;	//32?ID
	CAN_FilterInitStructure.CAN_FilterIdLow=0x0000;
	CAN_FilterInitStructure.CAN_FilterMaskIdHigh=0x0000;//32?MASK
	CAN_FilterInitStructure.CAN_FilterMaskIdLow=0x0000;
	CAN_FilterInitStructure.CAN_FilterFIFOAssignment=CAN_Filter_FIFO0;//???0???FIFO0
	CAN_FilterInitStructure.CAN_FilterActivation=ENABLE;//?????0

	CAN_FilterInit(&CAN_FilterInitStructure);			//??????
	
#if CAN_RX0_INT_ENABLE 
	CAN_ITConfig(CAN1,CAN_IT_FMP0,ENABLE);				//FIFO0????????.		    

	NVIC_InitStructure.NVIC_IRQChannel = USB_LP_CAN1_RX0_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;     // ?????1
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;            // ?????0
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
#endif
	return 0;
}   
 
#if CAN_RX0_INT_ENABLE	//??RX0??
//??????			    
void USB_LP_CAN1_RX0_IRQHandler(void)
{
  	CanRxMsg RxMessage;
	int i=0;
    CAN_Receive(CAN1, 0, &RxMessage);
	for(i=0;i<8;i++)
	printf("rxbuf[%d]:%d\r\n",i,RxMessage.Data[i]);
}
#endif

//===================================================================================
//can??????(????:ID?0X12,???,???)	
//len:????(???8)				     
//msg:????,???8???.
//???:0,??;
//		 ??,??;
u8 Can_Send_Msg(u8* msg,u8 len)
{	
	u8 mbox;
	u16 i=0;
	CanTxMsg TxMessage;
	TxMessage.StdId=0x12;			// ????? 
	TxMessage.ExtId=0x12;			// ??????? 
	TxMessage.IDE=CAN_Id_Standard; 	// ???
	TxMessage.RTR=CAN_RTR_Data;		// ???
	TxMessage.DLC=len;				// ????????
	for(i=0;i<len;i++)
	TxMessage.Data[i]=msg[i];			          
	mbox= CAN_Transmit(CAN1, &TxMessage);   
	i=0; 
	while((CAN_TransmitStatus(CAN1, mbox)==CAN_TxStatus_Failed)&&(i<0XFFF))i++;	//??????
	if(i>=0XFFF)return 1;
	return 0;	 
}

//===================================================================================
//can???????
//buf:?????;	 
//???:0,??????;
//		 ??,???????;
u8 Can_Receive_Msg(u8 *buf)
{		   		   
 	u32 i;
	CanRxMsg RxMessage;
    if( CAN_MessagePending(CAN1,CAN_FIFO0)==0)return 0;		//???????,???? 
    CAN_Receive(CAN1, CAN_FIFO0, &RxMessage);//????	
    for(i=0;i<8;i++)
    buf[i]=RxMessage.Data[i];  
	return RxMessage.DLC;	
}
















*/
