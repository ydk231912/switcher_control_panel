#ifndef _CAN_H
#define _CAN_H
#include "sys.h"

//CAN1����RX0�ж�ʹ��
#define CAN1_RX0_INT_ENABLE 0  //0��ʹ�ܣ�1ʹ��

#define CAN_RX_IRQHandler					USB_LP_CAN1_RX0_IRQHandler

u8 CAN1_Mode_Init(u8 tsjw,u8 tbs2,u8 tbs1,u16 brp,u8 mode);
u8 CAN1_Tx_Msg(u32 id,u8 ide,u8 rtr,u8 len,u8 *dat);		
u8 CAN1_Msg_Pend(u8 fifox);							
void CAN1_Rx_Msg(u8 fifox,u32 *id,u8 *ide,u8 *rtr,u8 *len,u8 *dat);
u8 CAN1_Tx_Staus(u8 mbox);  								
u8 CAN1_Send_Msg(u8* msg,u8 len);						
u8 CAN1_Receive_Msg(u8 *buf);						

u8 CAN1_Send_MsgTEST(u8* msg,u8 len);
u8 CAN1_Send_Num(u32 id,u8* msg);

void can_data_transition(void);

extern u8 cantxerr,BOF_F,EPV_F,EWG_F;

#endif
