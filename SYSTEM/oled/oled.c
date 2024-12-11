#include "oledfont.h"
#include "delay.h"
#include "oled.h"
u8 CSse=0; //��ʾ��ѡ��


//==========================================================
void OLED_Init(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOB|RCC_APB2Periph_GPIOC|RCC_APB2Periph_GPIOD|RCC_APB2Periph_GPIOE, ENABLE);	 //ʹ��A�˿�ʱ��
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);         //�˿���ӳ��
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);     //pin�˿�����
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3|GPIO_Pin_5;	 
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//�ٶ�50MHz
 	GPIO_Init(GPIOB, &GPIO_InitStructure);	  
	//==============================================================
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//�ٶ�50MHz
 	GPIO_Init(GPIOE, &GPIO_InitStructure);	 
 	GPIO_SetBits(GPIOE,GPIO_Pin_15);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8|GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//�ٶ�50MHz
 	GPIO_Init(GPIOD, &GPIO_InitStructure);	 
//====================================================================	

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10|GPIO_Pin_11|GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//�ٶ�50MHz
 	GPIO_Init(GPIOC, &GPIO_InitStructure);	 
 	GPIO_SetBits(GPIOC,GPIO_Pin_15);
	
//=============================================================================	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//�ٶ�50MHz
 	GPIO_Init(GPIOD, &GPIO_InitStructure);	
	
}



void initialize(void) //
{
	if(CSse==0){CS1=0;CS2=1;CS3=1;RES1=1;RES2=0;RES3=1;}
  if(CSse==1){CS1=1;CS2=0;CS3=1;RES1=1;RES2=1;RES3=1;} 
  if(CSse==2){CS1=1;CS2=1;CS3=0;RES1=1;RES2=1;RES3=1;}

	  delay_ms(100);	

	  OLED_WR_REG(0xae);//Set Display OFF //�ر���ʾ
		OLED_WR_REG(0xfd);//Set Command Lock //������
		OLED_WR_Byte(0x12); 
			
	if(USE_HORIZONTAL)
   {
	   OLED_WR_REG(0xA0);  
	   OLED_WR_Byte(0x24);	
	   OLED_WR_Byte(0x10);	

		OLED_WR_REG(0xa2);////A2.Display Offset
		OLED_WR_Byte(0x80); ////Offset:: 0~127				
   }
   else
   {
		OLED_WR_REG(0xA0);  
		OLED_WR_Byte(0x32);	
		OLED_WR_Byte(0x10);	

		OLED_WR_REG(0xa2);////A2.Display Offset
		OLED_WR_Byte(0x10); ////Offset:: 0~127		
   }
		
    OLED_WR_REG(0xa1);//Set Display Start Line ������ʾ��ʼ
   	OLED_WR_Byte(0x00); 
		
 //		OLED_WR_REG(0xa2);//Set Display Offset ������ʾƫ��
//		OLED_WR_Byte(0x00); 
		
		OLED_WR_REG(0xa6);//Normal Display   ������ʾ  ��ɫ����
		OLED_WR_REG(0xad);//Set IREF 
		OLED_WR_Byte(0x80); 
		 
		OLED_WR_REG(0xb1);//Set Reset (Phase 1)/Pre-charge (Phase 2)period ��������
		OLED_WR_Byte(0x74); 
		
		OLED_WR_REG(0xb3);//Set Front Clock Divider/Oscillator Frequency ����ʱ�ӷ�Ƶ
		OLED_WR_Byte(0xb1); 
		
		OLED_WR_REG(0xb6);//Set Second Pre-charge Period ���õڶ���Ԥ��
		OLED_WR_Byte(0x08); 
		
		OLED_WR_REG(0xb9);//Select Default Linear Gray Scale table ����Ĭ�����ԻҶ�
		OLED_WR_REG(0xba);//Set Pre-charge voltage configuration ���ó������
		OLED_WR_Byte(0x02); 
		
		OLED_WR_REG(0xbb);//Set Pre-charge voltage ���ó���ѹ
		OLED_WR_Byte(0x07); 
		
		OLED_WR_REG(0xbe);//Set VCOMH 
		OLED_WR_Byte(0x07); 
		
		OLED_WR_REG(0xc1);//Set Contrast Current ���öԱȶ�
		OLED_WR_Byte(0x4f);//VCC=14V 
		
    OLED_WR_REG(0xca);//Set MUX Ratio ���ö�·����
    OLED_WR_Byte(0x7f); 
   // CleanDDR(); 
	  clear( );// clear the whole DDRAM. ����ڴ�
    OLED_WR_REG(0xaf);//Set Display ON

}

void clear(void)
		{
		int i,j;
			
		OLED_WR_REG(0x15);//Set column address  �����е�ַ
		OLED_WR_REG(0x00);//Column Start Address ������ʼ��ַ
		OLED_WR_REG(0x3f);//Column End Address   ������ַ
		OLED_WR_REG(0x75);//Set row address   �е�ַ
		OLED_WR_REG(0x00);//Row Start Address   ����ʼ��ַ
		OLED_WR_REG(0x7f);//Row End Address    �н�����ַ
		for(i=0;i<128;i++)
		{
		for(j=0;j<64;j++)
		{
		 OLED_WR_Byte(0x00);
		}
		}

		}


//******************************************************************************
//    ����˵����OLEDд��һ��ָ��
//    ������ݣ�dat ����
//    ����ֵ��  ��
//******************************************************************************
void OLED_WR_Bus(u8 dat)
{
	u8 i;

	if(CSse==0){CS1=0;CS2=1;CS3=1;}
  if(CSse==1){CS1=1;CS2=0;CS3=1;}
  if(CSse==2){CS1=1;CS2=1;CS3=0;}
	for(i=0;i<8;i++)
	{			  
		SSCL=0;
		if(dat&0x80)SSDA=1;
		else SSDA=0;
		SSCL=1;
		dat<<=1;   
	}				 		  
	CS1=1;CS2=1;CS3=1;
}

//******************************************************************************
//    ����˵����OLEDд��һ��ָ��
//    ������ݣ�reg ָ��
//    ����ֵ��  ��
//******************************************************************************
void OLED_WR_REG(u8 reg)
{	  
	if(CSse==0){DC1=0;DC2=1;DC3=1;}
  if(CSse==1){DC1=1;DC2=0;DC3=1;}
  if(CSse==2){DC1=1;DC2=1;DC3=0;}
  OLED_WR_Bus(reg);
  DC1=1;DC2=1;DC3=1;	
}

//******************************************************************************
//    ����˵����OLEDд��һ������
//    ������ݣ�dat ����
//    ����ֵ��  ��
//******************************************************************************
void OLED_WR_Byte(u8 dat)
{	  
  OLED_WR_Bus(dat);
}
//******************************************************************************
//    ����˵����OLED��ʾ�е���ʼ��ֹ��ַ
//    ������ݣ�a  �е���ʼ��ַ
//              b  �е���ֹ��ַ
//    ����ֵ��  ��
//******************************************************************************
void Column_Address(u8 a,u8 b)
{
	OLED_WR_REG(0x15);       // Set Column Address
	OLED_WR_Byte(a+0x08);
	OLED_WR_Byte(b+0x08);
}


//******************************************************************************
//    ����˵����OLED��ʾ�е���ʼ��ֹ��ַ
//    ������ݣ�a  �е���ʼ��ַ
//              b  �е���ֹ��ַ
//    ����ֵ��  ��
//******************************************************************************
void Row_Address(u8 a,u8 b)
{
	OLED_WR_REG(0x75);       // Row Column Address
	OLED_WR_Byte(a);
	OLED_WR_Byte(b);
	OLED_WR_REG(0x5C);    //дRAM����
}

void OLED_Fill(u16 xstr,u16 ystr,u16 xend,u16 yend,u8 color)
{
	u8 x,y;
	xstr/=4; //column address ����4�� 
	xend/=4;
	Column_Address(xstr,xend-1);
	Row_Address(ystr,yend-1);
	for(x=xstr;x<xend;x++)
	{
		for(y=ystr;y<yend;y++)
		{
			OLED_WR_Byte(color); //����4�� ͬʱ֧��16�׻Ҷ� ������Ҫ2���ֽ�
			OLED_WR_Byte(color);
    }
  }
}
//******************************************************************************
//    ����˵����OLED��ʾ�ַ����� 
//    �˺������÷�Χ���ַ������4�ı���  �ַ��߶��ǿ�ȵ�2��
//    ������ݣ�x,y   ��ʼ����
//              chr   Ҫд����ַ�
//              sizey �ַ��߶� 
//              mode  0:������ʾ��1����ɫ��ʾ
//    ����ֵ��  ��
//******************************************************************************

void OLED_ShowChar(u8 x,u8 y,u8 chr,u8 sizey,u8 mode)
{
	u8 sizex,c,temp,t=2,DATA1=0,DATA2=0,m;
	u16 i,k,size2;
	x/=4;
	sizex=sizey/4/2; //��4����Ϊһ���е�ַ�߿���4�� ��2���ֿ�:�ָ� 1:2
	size2=(sizey/16+((sizey%16)?1:0))*sizey; //����һ���ַ���ռ�ֽ���
	c=chr-' '; //ASCII��ֵ��Ӧƫ�ƺ��ֵ
	Column_Address(x,x+sizex-1);//�����е�ַ
	Row_Address(y,y+sizey-1);//�����е�ַ
	for(i=0;i<size2;i++)
	{
		
		if(sizey==16)
		{
			temp=ascii_1608[c][i]; //��ȡȡģ���� 
		}
		else if(sizey==24)
		{
			temp=ascii_2412[c][i];//12x24 ASCII��
		}
		else if(sizey==32)
		{
			temp=ascii_3216[c][i];//16x32 ASCII��
		}
		if(sizey%16)
		{
			m=sizey/16+1;
			if(i%m) t=1;
			else t=2;
		}
		for(k=0;k<t;k++)
		{
			if(temp&(0x01<<(k*4)))
			{
				DATA1=0x0F;
			}
			if(temp&(0x01<<(k*4+1)))
			{
				DATA1|=0xF0;
			}
			if(temp&(0x01<<(k*4+2)))
			{
				DATA2=0x0F;
			}
			if(temp&(0x01<<(k*4+3)))
			{
				DATA2|=0xF0;
			}
			if(mode)
			{
				OLED_WR_Byte(~DATA2);
				OLED_WR_Byte(~DATA1);		
			}
			else
			{	
				OLED_WR_Byte(DATA2);
				OLED_WR_Byte(DATA1);
			}
			DATA1=0;
			DATA2=0;
		}
	}
}

//******************************************************************************
//    ����˵����OLED��ʾ�ַ���
//    ������ݣ�x,y  ��ʼ����
//              *dp   Ҫд����ַ�
//              sizey �ַ��߶� 
//              mode  0:������ʾ��1����ɫ��ʾ
//    ����ֵ��  ��
//******************************************************************************
void OLED_ShowString(u8 x,u8 y,u8 *dp,u8 sizey,u8 mode)
{
	while(*dp!='\0')
	{
	  OLED_ShowChar(x,y,*dp,sizey,mode);
		dp++;
		x+=sizey/2;
	}
}

//******************************************************************************
//    ����˵����m^n
//    ������ݣ�m:���� n:ָ��
//    ����ֵ��  result
//******************************************************************************
u32 oled_pow(u8 m,u8 n)
{
	u32 result=1;
	while(n--)result*=m;    
	return result;
}

//******************************************************************************
//    ����˵����OLED��ʾ����
//    ������ݣ�x,y :�������	 
//              num :Ҫ��ʾ�ı���
//              len :���ֵ�λ��
//              sizey �ַ��߶� 
//              mode  0:������ʾ��1����ɫ��ʾ
//    ����ֵ��  ��
//******************************************************************************
void OLED_ShowNum(u8 x,u8 y,u32 num,u8 len,u8 sizey,u8 mode)
{         	
	u8 t,temp;
	u8 enshow=0;
	for(t=0;t<len;t++)
	{
		temp=(num/oled_pow(10,len-t-1))%10;
		if(enshow==0&&t<(len-1))
		{
			if(temp==0)
			{
				OLED_ShowChar(x+(sizey/2)*t,y,' ',sizey,mode);
				continue;
			}else enshow=1; 
		 	 
		}
	 	OLED_ShowChar(x+(sizey/2)*t,y,temp+'0',sizey,mode); 
	}
}

//******************************************************************************


//******************************************************************************



//******************************************************************************
//    ����˵����OLED����ʼ��
//    ������ݣ���
//    ����ֵ��  ��
//******************************************************************************

	
		/*
		 OLED_CS_Clr(); 
	  OLED_RES_Set();	
	  delay_ms(10);	

	  OLED_WR_REG(0xae);//Set Display OFF //�ر���ʾ
		OLED_WR_REG(0xfd);//Set Command Lock //������
		OLED_WR_Byte(0x12); 
		
		OLED_WR_REG(0xa0);//Set Re-map and Dual COM Line mode ������ӳ��
		OLED_WR_Byte(0x14); 
		OLED_WR_Byte(0x10);
		
		
		
		OLED_WR_REG(0xa1);//Set Display Start Line ������ʾ��ʼ
		OLED_WR_Byte(0x00); 
		
		OLED_WR_REG(0xa2);//Set Display Offset ������ʾƫ��
		OLED_WR_Byte(0x00); 
		
		OLED_WR_REG(0xa6);//Normal Display   ������ʾ  ��ɫ����
		OLED_WR_REG(0xad);//Set IREF 
		OLED_WR_Byte(0x80); 
		 
		OLED_WR_REG(0xb1);//Set Reset (Phase 1)/Pre-charge (Phase 2)period ��������
		OLED_WR_Byte(0x74); 
		
		OLED_WR_REG(0xb3);//Set Front Clock Divider/Oscillator Frequency ����ʱ�ӷ�Ƶ
		OLED_WR_Byte(0xb1); 
		
		OLED_WR_REG(0xb6);//Set Second Pre-charge Period ���õڶ���Ԥ��
		OLED_WR_Byte(0x08); 
		
		OLED_WR_REG(0xb9);//Select Default Linear Gray Scale table ����Ĭ�����ԻҶ�
		OLED_WR_REG(0xba);//Set Pre-charge voltage configuration ���ó������
		OLED_WR_Byte(0x02); 
		
		OLED_WR_REG(0xbb);//Set Pre-charge voltage ���ó���ѹ
		OLED_WR_Byte(0x07); 
		
		OLED_WR_REG(0xbe);//Set VCOMH 
		OLED_WR_Byte(0x07); 
		
		OLED_WR_REG(0xc1);//Set Contrast Current ���öԱȶ�
		OLED_WR_Byte(0x4f);//VCC=14V 
		
    OLED_WR_REG(0xca);//Set MUX Ratio ���ö�·����
    OLED_WR_Byte(0x7f); 
   // CleanDDR(); 
	  clear( );// clear the whole DDRAM. ����ڴ�
    OLED_WR_REG(0xaf);//Set Display ON
		
		
		
		*/
		
		
		
		
		


