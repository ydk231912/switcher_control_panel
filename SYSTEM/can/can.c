#include "can.h"

/************************************************************
�������ܣ�CAN1��ʼ��
��ڲ�����tsjw:����ͬ����Ծʱ�䵥Ԫ����Χ1~3
          tbs2��ʱ���2��ʱ�䵥Ԫ����Χ1~8
		  tbs1��ʱ���1��ʱ�䵥Ԫ����Χ1~16
		  brp�������ʷ���������Χ��1~1024����ʵ��Ҫ��1��Ҳ����1~1024��tq=��brp��*tpclk1
		  mode��0����ͨģʽ��1�ػ�ģʽ
����ֵ��0��ʼ���ɹ���������Ϊ��ʼ��ʧ��
ע�⣺����mode����Ϊ0������������ԣ�
������=Fpclk/����tbs1+tbs2+1��*brp����FpclkΪ36MHz
      =36/����3+2+1��*6��=1MHz
*************************************************************/
u8 cantxerr=0,BOF_F=0,EPV_F=0,EWG_F=0;

u8 CAN1_Mode_Init(u8 tsjw,u8 tbs2,u8 tbs1,u16 brp,u8 mode)
{
	GPIO_InitTypeDef GPIO_InitStructure;
//NVIC_InitTypeDef NVIC_InitStructure;
	CAN_FilterInitTypeDef CAN_FilterInitStructure;
	u16 i=0;
 	if(tsjw==0||tbs2==0||tbs1==0||brp==0)return 1;
	tsjw-=1; //Subtract 1 before setting //�ȼ�ȥ1.����������
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
	
	CAN1->MCR=0X0000;//�˳�˯��ģʽ
	CAN1->MCR|=1<<0;//����can1�����ʼ��ģʽ
	while((CAN1->MCR&1<<0)==0)
	{
		i++;
		if(i>100) return 2;//�����ʼ��ģʽʧ��
	}
	CAN1->MCR|=0<<7;//��ʱ�䴥��ͨ��ģʽ
	CAN1->MCR|=1<<6;//����Զ����߹���
//CAN1->MCR|=1<<4;//���Ʊ����Զ�����
	CAN1->MCR|=1<<5;  //˯��ͨ�����Ļ���
	CAN1->MCR|=0<<3;//���Ĳ��������µĸ��Ǿ͵�
	CAN1->MCR|=0<<2;//���ȼ��ɱ��ı�ʶ������
	CAN1->MCR&=~(1<<1);//�������
	CAN1->BTR =0x00000000;//���ԭ��������
	CAN1->BTR|=mode<<30;//����ģʽ��0Ϊ��ͨģʽ��1Ϊ�ػ�ģʽ
	CAN1->BTR|=tsjw<<24;//����ͬ����Ծ���Ϊtsjw+1����λʱ��
	CAN1->BTR|=tbs2<<20;//tbs2=tbs2+1��ʱ�䵥λ
	CAN1->BTR|=tbs1<<16;//tsb1=tsb1+1��ʱ�䵥λ
	CAN1->BTR|=brp<<0;//��Ƶϵ��Ϊbrp+1,������Ϊ36MHz/��tbs1+tbs2+1��/fdiv
	CAN1->MCR&=~(1<<0);
	while((CAN1->MCR&1<<0)==1)
	{
		i++;
		if(i>0XFFF0) return 3;//�˳���ʼ��ģʽʧ��
	}
	CAN_FilterInitStructure.CAN_FilterNumber = 1;//ָ��������Ϊ1
	CAN_FilterInitStructure.CAN_FilterMode   = CAN_FilterMode_IdMask;//ָ��������Ϊ��ʶ������λģʽ
	CAN_FilterInitStructure.CAN_FilterScale  = CAN_FilterScale_32bit;//������λ��Ϊ32λ
	CAN_FilterInitStructure.CAN_FilterIdHigh = 0x0000;//��������ʶ���ĸ�ʮ��λ
	CAN_FilterInitStructure.CAN_FilterMaskIdLow=0x0000;//��������ʶ���ĵ�ʮ��λ
	CAN_FilterInitStructure.CAN_FilterMaskIdHigh=0x0000;//���������α�ʶ���ĸ�ʮ��λ
	CAN_FilterInitStructure.CAN_FilterMaskIdLow =0x0000;//���������α�ʶ���ĵ�ʮ��λ
	CAN_FilterInitStructure.CAN_FilterFIFOAssignment=CAN_FIFO0;//ָ�����˵�FIFOΪ0
	CAN_FilterInitStructure.CAN_FilterActivation=ENABLE;//ʹ�ܹ�����
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

/**************************************************************************
Function: CAN sends data
Input   : id:Standard ID(11 bits)/ Extended ID(11 bits +18 bits)
			    ide:0, standard frame;1, extension frames
			    rtr:0, data frame;1, remote frame
			    len:Length of data to be sent (fixed at 8 bytes, valid data is 6 bytes in time-triggered mode) 
			    *dat:Pointer to the data
Output  : 0~3, mailbox number. 0xFF, no valid mailbox
�������ܣ�CAN��������
��ڲ�����id:��׼ID(11λ)/��չID(11λ+18λ)	    
			    ide:0,��׼֡;1,��չ֡
			    rtr:0,����֡;1,Զ��֡
			    len:Ҫ���͵����ݳ���(�̶�Ϊ8���ֽ�,��ʱ�䴥��ģʽ��,��Ч����Ϊ6���ֽ�)
			    *dat:����ָ��.
����  ֵ��0~3,������.0XFF,����Ч����
**************************************************************************/
u8 CAN1_Tx_Msg(u32 id,u8 ide,u8 rtr,u8 len,u8 *dat)
{
	u8 mbox;
	if(CAN1->TSR&(1<<26)) mbox=0;//����oΪ��
	else if(CAN1->TSR&(1<<27)) mbox=1;//����1Ϊ��
	else if(CAN1->TSR&(1<<28)) mbox=2;//����2Ϊ��
	else return 0XFF;//�޿����䣬�޷�����
	
	CAN1->sTxMailBox[mbox].TIR=0;//���֮ǰ������
	if(ide==0)//��׼֡
	{
		id&=0x7ff;//ȡ�õ�11λ��stdid
		id<<=21;
	}
	else//��չ֡
	{
		id&=0X1FFFFFFF;//ȡ�õ�32λ��extid
		id<<=3;
	}
	CAN1->sTxMailBox[mbox].TIR|=id;
	CAN1->sTxMailBox[mbox].TIR|=ide<<2;
	CAN1->sTxMailBox[mbox].TIR|=rtr<<1;
	len&=0X0F;//�õ�����λ
	CAN1->sTxMailBox[mbox].TDTR&=~(0X0000000F);
	CAN1->sTxMailBox[mbox].TDTR|=len;//����DLC
	//���������ݴ�������
	CAN1->sTxMailBox[mbox].TDHR=(((u32)dat[7]<<24)|
								((u32)dat[6]<<16)|
 								((u32)dat[5]<<8)|
								((u32)dat[4]));
	CAN1->sTxMailBox[mbox].TDLR=(((u32)dat[3]<<24)|
								((u32)dat[2]<<16)|
 								((u32)dat[1]<<8)|
								((u32)dat[0]));
	CAN1->sTxMailBox[mbox].TIR|=1<<0; //Request to send mailbox data//��������������
	return mbox;
}

/**************************************************************************
Function: Get the send status
Input   : Mbox: mailbox number
Output  : 0, hang;0X05, send failed;0X07, successful transmission
�������ܣ���÷���״̬
��ڲ�����mbox��������
����  ֵ��0,����;0X05,����ʧ��;0X07,���ͳɹ�
**************************************************************************/
u8 CAN1_Tx_Staus(u8 mbox)
{	
	u8 sta=0;					    
	switch (mbox)
	{
		case 0: 
			sta |= CAN1->TSR&(1<<0);			   //RQCP0
			sta |= CAN1->TSR&(1<<1);			   //TXOK0
			sta |=((CAN1->TSR&(1<<26))>>24); //TME0
			break;
		case 1: 
			sta |= CAN1->TSR&(1<<8)>>8;		   //RQCP1
			sta |= CAN1->TSR&(1<<9)>>8;		   //TXOK1
			sta |=((CAN1->TSR&(1<<27))>>25); //TME1	   
			break;
		case 2: 
			sta |= CAN1->TSR&(1<<16)>>16;	   //RQCP2
			sta |= CAN1->TSR&(1<<17)>>16;	   //TXOK2
			sta |=((CAN1->TSR&(1<<28))>>26); //TME2
			break;
		default:
			sta=0X05; //Wrong email number, failed //����Ų���,ʧ��
		break;
	}
	return sta;
} 
/**************************************************************************
Function: Returns the number of packets received in FIFO0/FIFO1
Input   : Fifox: FIFO number (0, 1)
Output  : Number of packets in FIFO0/FIFO1
�������ܣ��õ���FIFO0/FIFO1�н��յ��ı��ĸ���
��ڲ�����fifox��FIFO��ţ�0��1��
����  ֵ��FIFO0/FIFO1�еı��ĸ���
**************************************************************************/
u8 CAN1_Msg_Pend(u8 fifox)
{
	if(fifox==0)return CAN1->RF0R&0x03; 
	else if(fifox==1)return CAN1->RF1R&0x03; 
	else return 0;
}
/**************************************************************************
Function: Receive data
Input   : fifox��Email
		    	id:Standard ID(11 bits)/ Extended ID(11 bits +18 bits)
			    ide:0, standard frame;1, extension frames 
			    rtr:0, data frame;1, remote frame
			    len:Length of data received (fixed at 8 bytes, valid at 6 bytes in time-triggered mode)
			    dat:Data cache
Output  : none
�������ܣ���������
��ڲ�����fifox�������
		    	id:��׼ID(11λ)/��չID(11λ+18λ)
			    ide:0,��׼֡;1,��չ֡
			    rtr:0,����֡;1,Զ��֡
			    len:���յ������ݳ���(�̶�Ϊ8���ֽ�,��ʱ�䴥��ģʽ��,��Ч����Ϊ6���ֽ�)
			    dat:���ݻ�����
����  ֵ���� 
**************************************************************************/
void CAN1_Rx_Msg(u8 fifox,u32 *id,u8 *ide,u8 *rtr,u8 *len,u8 *dat)
{	   
	*ide=CAN1->sFIFOMailBox[fifox].RIR&0x04; //Gets the value of the identifier selection bit //�õ���ʶ��ѡ��λ��ֵ  
 	if(*ide==0) //Standard identifier //��׼��ʶ��
	{
		*id=CAN1->sFIFOMailBox[fifox].RIR>>21;
	}else	     //Extended identifier //��չ��ʶ��
	{
		*id=CAN1->sFIFOMailBox[fifox].RIR>>3;
	}
	*rtr=CAN1->sFIFOMailBox[fifox].RIR&0x02;	//Gets the remote send request value //�õ�Զ�̷�������ֵ
	*len=CAN1->sFIFOMailBox[fifox].RDTR&0x0F; //Get the DLC //�õ�DLC
 	//*fmi=(CAN1->sFIFOMailBox[FIFONumber].RDTR>>8)&0xFF; //Get the FMI //�õ�FMI
	//Receive data //��������
	dat[0]=CAN1->sFIFOMailBox[fifox].RDLR&0XFF;
	dat[1]=(CAN1->sFIFOMailBox[fifox].RDLR>>8)&0XFF;
	dat[2]=(CAN1->sFIFOMailBox[fifox].RDLR>>16)&0XFF;
	dat[3]=(CAN1->sFIFOMailBox[fifox].RDLR>>24)&0XFF;    
	dat[4]=CAN1->sFIFOMailBox[fifox].RDHR&0XFF;
	dat[5]=(CAN1->sFIFOMailBox[fifox].RDHR>>8)&0XFF;
	dat[6]=(CAN1->sFIFOMailBox[fifox].RDHR>>16)&0XFF;
	dat[7]=(CAN1->sFIFOMailBox[fifox].RDHR>>24)&0XFF;    
  if(fifox==0)CAN1->RF0R|=0X20;      //Free the FIFO0 mailbox //�ͷ�FIFO0����
	else if(fifox==1)CAN1->RF1R|=0X20; //Free the FIFO1 mailbox //�ͷ�FIFO1����	 
}
/**************************************************************************
Function: CAN receives interrupt service function, conditional compilation
Input   : none
Output  : none
�������ܣ�CAN�����жϷ���������������
��ڲ�������
����  ֵ���� 
**************************************************************************/
#if CAN1_RX0_INT_ENABLE	//Enable RX0 interrupt //ʹ��RX0�ж�	    
//void CAN_RX_IRQHandler(void)
//{
////  u8 i;
//	u32 id;
//	u8 ide,rtr,len;     
////	u8 ON_rxbuf[8]={10,12,15,19,24,30,37,0};У������

//	u8 temp_rxbuf[8];
//	//Verify the first set of data, and CAN control mode will be started if the verification is successful
//	//У���һ�����ݣ�У��ɹ�����CAN����ģʽ
// 	CAN1_Rx_Msg(0,&id,&ide,&rtr,&len,temp_rxbuf);
//	if(id == Ctrl_ID)
//	{
//		 Ctrl_Mode      = temp_rxbuf[0];//����ģʽ
//		 DIR_Receive    = temp_rxbuf[1];//���յ����������ת��
//		 Microstep      = temp_rxbuf[2];//���յ����������ϸ��ֵ
//		 switch(Ctrl_Mode)
//		 { 
//			 case Speed_Mode:
//				 SPEED =(float)SPeed_transition(temp_rxbuf[5],temp_rxbuf[6])/6.28318531;;//���ܵ�����������ٶȣ�����Ȧ/s
//				 break;
//			 case Position_Mode:
//				 if(DIR_Receive==0)
//				    Loop_T_Position =Loop_T_Position+(int)(17.77778*Microstep/32*Position_transition(temp_rxbuf[3],temp_rxbuf[4])/10.0);
//				 else
//					Loop_T_Position =Loop_T_Position-(int)(17.77778*Microstep/32*Position_transition(temp_rxbuf[3],temp_rxbuf[4])/10.0);
//			     SPEED =(float)SPeed_transition(temp_rxbuf[5],temp_rxbuf[6])/6.28318531;;//���ܵ�����������ٶȣ�����Ȧ/s
//				 break;
//			 case Torque_Mode:
//				 Current_value = temp_rxbuf[3];
//			     SPEED =(float)SPeed_transition(temp_rxbuf[5],temp_rxbuf[6])/6.28318531;;//���ܵ�����������ٶȣ�����Ȧ/s
//				break;
//			 case Absolute_Angle_Mode:
//				 Absolute_T_Position = Angle_transition(temp_rxbuf[3],temp_rxbuf[4]);
//			     SPEED =(float)SPeed_transition(temp_rxbuf[5],temp_rxbuf[6])/6.28318531;;//���ܵ�����������ٶȣ�����Ȧ/s
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

/**************************************************************************
Function: CAN1 sends a set of data (fixed format :ID 0X601, standard frame, data frame)
Input   : msg:Pointer to the data
    			len:Data length (up to 8)
Output  : 0, success, others, failure;
�������ܣ�CAN1����һ������(�̶���ʽ:IDΪ0X601,��׼֡,����֡)
��ڲ�����msg:����ָ��
    			len:���ݳ���(���Ϊ8)
����  ֵ��0,�ɹ�������,ʧ��;
**************************************************************************/
u8 CAN1_Send_Msg(u8* msg,u8 len)
{	
	u8 mbox;
	u16 i=0;	  	 						       
    mbox=CAN1_Tx_Msg(0X601,0,0,len,msg);   
	while((CAN1_Tx_Staus(mbox)!=0X07)&&(i<0XFFF))i++; //Waiting for the end of sending //�ȴ����ͽ���
	if(i>=0XFFF)return 1; //Send failure //����ʧ��
	return 0;	//Send a success //���ͳɹ�									
}
/**************************************************************************
Function: The CAN1 port receives data queries
Input   : Buf: The data cache
Output  : 0, number of data received, other, length of data received
�������ܣ�CAN1�ڽ������ݲ�ѯ
��ڲ�����buf:���ݻ�����
����  ֵ��0,�����ݱ��յ�������,���յ����ݳ���
**************************************************************************/
u8 CAN1_Receive_Msg(u8 *buf)
{		   		   
	u32 id;
	u8 ide,rtr,len; 
	if(CAN1_Msg_Pend(0)==0)return 0;			   //No data received, exit directly //û�н��յ�����,ֱ���˳� 	 
  	CAN1_Rx_Msg(0,&id,&ide,&rtr,&len,buf); //Read the data //��ȡ����
    if(id!=0x12||ide!=0||rtr!=0)len=0;		 //Receive error //���մ���	   
	return len;	
}
/**************************************************************************
Function: Can1 sends a set of data tests
Input   : msg:Pointer to the data
			    len:Data length (up to 8)
Output  : 0, success, 1, failure
�������ܣ�CAN1����һ�����ݲ���
��ڲ�����msg:����ָ��
			    len:���ݳ���(���Ϊ8)
����  ֵ��0,�ɹ���1,ʧ��
**************************************************************************/
u8 CAN1_Send_MsgTEST(u8* msg,u8 len)
{	
	u8 mbox;
	u16 i=0;	  	 						       
    mbox=CAN1_Tx_Msg(0X701,0,0,len,msg);   
	while((CAN1_Tx_Staus(mbox)!=0X07)&&(i<0XFFF))i++; //Waiting for the end of sending //�ȴ����ͽ���
	if(i>=0XFFF)return 1;	//Send failure //����ʧ��
	return 0;	//Send a success //���ͳɹ�
}
/**************************************************************************
Function: Sends an array to the given ID
Input   : id��ID no.
			    msg��The transmitted data pointer
Output  : 0, success, 1, failure
�������ܣ���������id����һ�����������
��ڲ�����id��ID��
			    msg������������ָ��
����  ֵ��0,�ɹ���1,ʧ��
**************************************************************************/
u8 CAN1_Send_Num(u32 id,u8* msg)
{
	u8 mbox;
	u16 i=0;	  	 						       
  mbox=CAN1_Tx_Msg(id,0,0,8,msg);   
	while((CAN1_Tx_Staus(mbox)!=0X07)&&(i<0XFFF))i++; //Waiting for the end of sending //�ȴ����ͽ���
	if(i>=0XFFF)return 1;	//Send failure //����ʧ��
	return 0;
}


void CAN1_err(void)
{
  u32 temp;
	temp=(CAN1->ESR)>>=16;
	cantxerr=temp&0x00ff;
  BOF_F=(CAN1->ESR>>2)&0x01;  //CAN����״̬
	EPV_F=(CAN1->ESR>>1)&0x01; //��������ﵽ���󱻶���ֵ
	EWG_F=(CAN1->ESR>>0)&0x01; //��������ﵽ����ֵ
	
	CAN1->MCR&=~(1<<1);//�������˯��
	
	
}


/*


#include "can.h"
#include "led.h"
#include "delay.h"
//#include "usart.h"
//////////////////////////////////////////////////////////////////////////////////	

//??:??????????

////////////////////////////////////////////////////////////////////////////////// 	 
 
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
