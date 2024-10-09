#include  "e8266.h"


/*

const u8* portnum="8086";		 

//
const u8* wifista_ssid="Sew_5G";			//
const u8* wifista_encryption="wpawpa2_aes";	//
const u8* wifista_password="Sew15166665554~"; 	//

//WIFI AP??,?????????,?????.
const u8* wifiap_ssid="ATKESP8266";			//
const u8* wifiap_encryption="wpawpa2_aes";	//
const u8* wifiap_password="12345678a"; 		//






u8 atk_8266_send_cmd(u8 *cmd,u8 *ack,u16 waittime)
{
	u8 res=0; 
	USART2_RX_STA=0;
	u2_printf("%s\r\n",cmd);	//????
	if(ack&&waittime)		//??????
	{
		while(--waittime)	//?????
		{
			delay_ms(10);
			if(USART2_RX_STA&0X8000)//??????????
			{
				if(atk_8266_check_cmd(ack))
				{
					printf("ack:%s\r\n",(u8*)ack);
					break;//?????? 
				}
					USART2_RX_STA=0;
			} 
		}
		if(waittime==0)res=1; 
	}
	return res;
} 

*/














