#include "EPD_GUI.h"
#include "EPD_Font.h"

PAINT Paint;


/*******************************************************************
Function Description: Create an image cache array
Interface Description: * Image: The image array to be passed in
Width Image Width
Heighe image length
Rotate screen display orientation
Color Display Color
Return value: None
*******************************************************************/
void Paint_NewImage(uint8_t *image,uint16_t Width,uint16_t Height,uint16_t Rotate,uint16_t Color)
{
	Paint.Image = 0x00;
	Paint.Image = image;
	Paint.color = Color;  
	Paint.widthMemory = Width;
	Paint.heightMemory = Height;  
	Paint.widthByte = (Width % 8 == 0)? (Width / 8 ): (Width / 8 + 1);
	Paint.heightByte = Height;     
	Paint.rotate = Rotate;
	if(Rotate==0||Rotate==180) 
	{
		Paint.width=Height;
		Paint.height=Width;
	} 
	else 
	{
		Paint.width = Width;
		Paint.height = Height;
	}
}				 

/*******************************************************************
Function Description: Clear Buffer
Interface Description: Color Pixel Color Parameters
Return value: None
*******************************************************************/
void Paint_Clear(uint8_t Color)
{
	uint16_t X,Y;
	uint32_t Addr;
  for(Y=0;Y<Paint.heightByte;Y++) 
	{
    for(X=0;X<Paint.widthByte;X++) 
		{   
      Addr=X+Y*Paint.widthByte;//8 pixel =  1 byte
      Paint.Image[Addr]=Color;
    }
  }
}


/*******************************************************************
Function description: Light up a pixel point
Interface description: Xpoint pixel x-coordinate parameters
Ypoint pixel point Y coordinate parameter
Color Pixel Color Parameters
Return value: None
*******************************************************************/
void Paint_SetPixel(uint16_t Xpoint,uint16_t Ypoint,uint16_t Color)
{
	uint16_t X, Y;
	uint32_t Addr;
	uint8_t Rdata;
    switch(Paint.rotate) 
		{
			case 0:
					X=Ypoint;
					Y=Xpoint;		
					break;
			case 90:
					X=Xpoint;
					Y=Paint.heightMemory-Ypoint-1;
					break;
			case 180:
					X=Paint.widthMemory-Ypoint-1;
					Y=Paint.heightMemory-Xpoint-1;
					break;
			case 270:
					X=Paint.widthMemory-Xpoint-1;
					Y=Ypoint;
					break;
				default:
						return;
    }
		Addr=X/8+Y*Paint.widthByte;
    Rdata=Paint.Image[Addr];
    if(Color==BLACK)
    {    
			Paint.Image[Addr]=Rdata&~(0x80>>(X % 8)); //����Ӧ����λ��0
		}
    else
		{
      Paint.Image[Addr]=Rdata|(0x80>>(X % 8));   //����Ӧ����λ��1  
		}
}


/*******************************************************************
Function description: underlined function
Interface description: Xstart pixel x starting coordinate parameter
Ystart pixel Y starting coordinate parameter
Xend pixel x end coordinate parameter
End coordinate parameter for Yend pixel Y
Color Pixel Color Parameters
Return value: None
*******************************************************************/
void EPD_DrawLine(uint16_t Xstart,uint16_t Ystart,uint16_t Xend,uint16_t Yend,uint16_t Color)
{   
	uint16_t Xpoint, Ypoint;
	int dx, dy;
	int XAddway,YAddway;
	int Esp;
	char Dotted_Len;
  Xpoint = Xstart;
  Ypoint = Ystart;
  dx = (int)Xend - (int)Xstart >= 0 ? Xend - Xstart : Xstart - Xend;
  dy = (int)Yend - (int)Ystart <= 0 ? Yend - Ystart : Ystart - Yend;
  XAddway = Xstart < Xend ? 1 : -1;
  YAddway = Ystart < Yend ? 1 : -1;
  Esp = dx + dy;
  Dotted_Len = 0;
  for (;;) {
        Dotted_Len++;
            Paint_SetPixel(Xpoint, Ypoint, Color);
        if (2 * Esp >= dy) {
            if (Xpoint == Xend)
                break;
            Esp += dy;
            Xpoint += XAddway;
        }
        if (2 * Esp <= dx) {
            if (Ypoint == Yend)
                break;
            Esp += dx;
            Ypoint += YAddway;
        }
    }
}
/*******************************************************************
Function Description: Draw a Rectangular Function
Interface description: Xstart rectangle x starting coordinate parameter
Ystart rectangle Y starting coordinate parameters
Xend rectangle x end coordinate parameter
End coordinate parameters of Yend rectangle Y
Color Pixel Color Parameters
Is the mode rectangle filled
Return value: None
*******************************************************************/
void EPD_DrawRectangle(uint16_t Xstart,uint16_t Ystart,uint16_t Xend,uint16_t Yend,uint16_t Color,uint8_t mode)
{
	uint16_t i;
    if (mode)
			{
        for(i = Ystart; i < Yend; i++) 
				{
          EPD_DrawLine(Xstart,i,Xend,i,Color);
        }
      }
		else 
		 {
        EPD_DrawLine(Xstart, Ystart, Xend, Ystart, Color);
        EPD_DrawLine(Xstart, Ystart, Xstart, Yend, Color);
        EPD_DrawLine(Xend, Yend, Xend, Ystart, Color);
        EPD_DrawLine(Xend, Yend, Xstart, Yend, Color);
		 }
}
/*******************************************************************
Function description: Draw a circular function
Interface Description: X_Center Center Center x Starting Coordinate Parameter
YCenter center point Y coordinate parameter
Radius circular radius parameter
Color Pixel Color Parameters
Does the mode circle fill the display
Return value: None
*******************************************************************/
void EPD_DrawCircle(uint16_t X_Center,uint16_t Y_Center,uint16_t Radius,uint16_t Color,uint8_t mode)
{
	int Esp, sCountY;
	uint16_t XCurrent, YCurrent;
  XCurrent = 0;
  YCurrent = Radius;
  Esp = 3 - (Radius << 1 );
    if (mode) {
        while (XCurrent <= YCurrent ) { //Realistic circles
            for (sCountY = XCurrent; sCountY <= YCurrent; sCountY ++ ) {
                Paint_SetPixel(X_Center + XCurrent, Y_Center + sCountY, Color);//1
                Paint_SetPixel(X_Center - XCurrent, Y_Center + sCountY, Color);//2
                Paint_SetPixel(X_Center - sCountY, Y_Center + XCurrent, Color);//3
                Paint_SetPixel(X_Center - sCountY, Y_Center - XCurrent, Color);//4
                Paint_SetPixel(X_Center - XCurrent, Y_Center - sCountY, Color);//5
                Paint_SetPixel(X_Center + XCurrent, Y_Center - sCountY, Color);//6
                Paint_SetPixel(X_Center + sCountY, Y_Center - XCurrent, Color);//7
                Paint_SetPixel(X_Center + sCountY, Y_Center + XCurrent, Color);
            }
            if ((int)Esp < 0 )
                Esp += 4 * XCurrent + 6;
            else {
                Esp += 10 + 4 * (XCurrent - YCurrent );
                YCurrent --;
            }
            XCurrent ++;
        }
    } else { //Draw a hollow circle
        while (XCurrent <= YCurrent ) {
            Paint_SetPixel(X_Center + XCurrent, Y_Center + YCurrent, Color);//1
            Paint_SetPixel(X_Center - XCurrent, Y_Center + YCurrent, Color);//2
            Paint_SetPixel(X_Center - YCurrent, Y_Center + XCurrent, Color);//3
            Paint_SetPixel(X_Center - YCurrent, Y_Center - XCurrent, Color);//4
            Paint_SetPixel(X_Center - XCurrent, Y_Center - YCurrent, Color);//5
            Paint_SetPixel(X_Center + XCurrent, Y_Center - YCurrent, Color);//6
            Paint_SetPixel(X_Center + YCurrent, Y_Center - XCurrent, Color);//7
            Paint_SetPixel(X_Center + YCurrent, Y_Center + XCurrent, Color);//0
            if ((int)Esp < 0 )
                Esp += 4 * XCurrent + 6;
            else {
                Esp += 10 + 4 * (XCurrent - YCurrent );
                YCurrent --;
            }
            XCurrent ++;
        }
    }
}

/******************************************************************************
Function Description: Display Chinese character strings
Entrance data: x, y display coordinates
*The Chinese character string to be displayed
Sizey font size
Color Text Color
Return value: None
******************************************************************************/
void EPD_ShowChinese(uint16_t x,uint16_t y,uint8_t *s,uint8_t sizey,uint16_t color)
{
	while(*s!=0)
	{
		if(sizey==12) EPD_ShowChinese12x12(x,y,s,sizey,color);
		else if(sizey==16) EPD_ShowChinese16x16(x,y,s,sizey,color);
		else if(sizey==24) EPD_ShowChinese24x24(x,y,s,sizey,color);
		else if(sizey==32) EPD_ShowChinese32x32(x,y,s,sizey,color);
		else return;
		s+=2;
		x+=sizey;
	}
}

/******************************************************************************
Function Description: Display a single 12x12 Chinese character
Entrance data: x, y display coordinates
*The Chinese characters to be displayed
Sizey font size
Color Text Color
Return value: None
******************************************************************************/
void EPD_ShowChinese12x12(uint16_t x,uint16_t y,uint8_t *s,uint8_t sizey,uint16_t color)
{
	uint8_t i,j;
	uint16_t k;
	uint16_t HZnum;//Number of Chinese characters
	uint16_t TypefaceNum;//The byte size occupied by one character
	uint16_t x0=x;
	TypefaceNum=(sizey/8+((sizey%8)?1:0))*sizey;                    
	HZnum=sizeof(tfont12)/sizeof(typFNT_GB12);	//Count the number of Chinese characters
	for(k=0;k<HZnum;k++) 
	{
		if((tfont12[k].Index[0]==*(s))&&(tfont12[k].Index[1]==*(s+1)))
		{ 	
			for(i=0;i<TypefaceNum;i++)
			{
				for(j=0;j<8;j++)
				{	
						if(tfont12[k].Msk[i]&(0x01<<j))	Paint_SetPixel(x,y,color);//Draw a dot
						x++;
						if((x-x0)==sizey)
						{
							x=x0;
							y++;
							break;
						}
				}
			}
		}				  	
		continue;  //If the corresponding dot matrix font library is found, exit immediately to prevent the impact of multiple Chinese characters being duplicated
	}
} 

/******************************************************************************
Function Description: Display a single 16x16 Chinese character
Entrance data: x, y display coordinates
*The Chinese characters to be displayed
Sizey font size
Color Text Color
Return value: None
******************************************************************************/
void EPD_ShowChinese16x16(uint16_t x,uint16_t y,uint8_t *s,uint8_t sizey,uint16_t color)
{
	uint8_t i,j;
	uint16_t k;
	uint16_t HZnum;//Number of Chinese characters
	uint16_t TypefaceNum;//The byte size occupied by one character
	uint16_t x0=x;
  TypefaceNum=(sizey/8+((sizey%8)?1:0))*sizey;
	HZnum=sizeof(tfont16)/sizeof(typFNT_GB16);	//Count the number of Chinese characters
	for(k=0;k<HZnum;k++) 
	{
		if ((tfont16[k].Index[0]==*(s))&&(tfont16[k].Index[1]==*(s+1)))
		{ 	
			for(i=0;i<TypefaceNum;i++)
			{
				for(j=0;j<8;j++)
				{	
						if(tfont16[k].Msk[i]&(0x01<<j))	Paint_SetPixel(x,y,color);//Draw a dot
						x++;
						if((x-x0)==sizey)
						{
							x=x0;
							y++;
							break;
						}
				}
			}
			break;
		}				  	
		continue; //If the corresponding dot matrix font library is found, exit immediately to prevent the impact of multiple Chinese characters being duplicated
	}
} 
void EPD_ShowChinese16x16_UTF8(uint16_t x, uint16_t y, uint8_t *s, uint8_t sizey, uint16_t color)
{
    uint8_t i,j;
    uint16_t k;
    uint16_t HZnum;
    uint16_t TypefaceNum;
    uint16_t x0 = x;
    
    TypefaceNum = (sizey/8 + ((sizey%8)?1:0)) * sizey;
    HZnum = sizeof(tfont16)/sizeof(typFNT_GB16);
    
    // 检查是否是UTF-8中文字符
    if ((s[0] & 0xE0) != 0xE0) {
        ESP_LOGE("TAG", "不是UTF-8中文字符: 0x%02X", s[0]);
        return;
    }
    
  //  ESP_LOGI("TAG", "查找UTF-8汉字: 0x%02X%02X%02X", s[0], s[1], s[2]);
    
    for(k = 0; k < HZnum; k++) 
    {
        // UTF-8编码比较（3个字节）
        if (tfont16[k].Index[0] == s[0] &&
            tfont16[k].Index[1] == s[1] &&
            tfont16[k].Index[2] == s[2])
        { 	
          //  ESP_LOGI("TAG", "找到汉字: %s", tfont16[k].Index);
            
            for(i = 0; i < TypefaceNum; i++)
            {
                for(j = 0; j < 8; j++)
                {	
                    if(tfont16[k].Msk[i] & (0x01 << j)) {
                        Paint_SetPixel(x, y, color);
                    }
                    x++;
                    if((x - x0) == sizey)
                    {
                        x = x0;
                        y++;
                        break;
                    }
                }
            }
            break;  // 找到后立即退出
        }
    }
}

/******************************************************************************
Function Description: Display a single 24x24 Chinese character
Entrance data: x, y display coordinates
*The Chinese characters to be displayed
Sizey font size
Color Text Color
Return value: None
******************************************************************************/
void EPD_ShowChinese24x24(uint16_t x,uint16_t y,uint8_t *s,uint8_t sizey,uint16_t color)
{
	uint8_t i,j;
	uint16_t k;
	uint16_t HZnum;//Number of Chinese characters
	uint16_t TypefaceNum;//The byte size occupied by one character
	uint16_t x0=x;
	TypefaceNum=(sizey/8+((sizey%8)?1:0))*sizey;
	HZnum=sizeof(tfont24)/sizeof(typFNT_GB24);	//Count the number of Chinese characters
	for(k=0;k<HZnum;k++) 
	{
		if ((tfont24[k].Index[0]==*(s))&&(tfont24[k].Index[1]==*(s+1)))
		{ 	
			for(i=0;i<TypefaceNum;i++)
			{
				for(j=0;j<8;j++)
				{	
					if(tfont24[k].Msk[i]&(0x01<<j))	Paint_SetPixel(x,y,color);//Draw a dot
					x++;
					if((x-x0)==sizey)
					{
						x=x0;
						y++;
						break;
					}
				}
			}
		}				  	
		continue;  //If the corresponding dot matrix font library is found, exit immediately to prevent the impact of multiple Chinese characters being duplicated
	}
} 

/******************************************************************************
Function Description: Display a single 32x32 Chinese character
Entrance data: x, y display coordinates
*The Chinese characters to be displayed
Sizey font size
Color Text Color
Return value: None
******************************************************************************/
void EPD_ShowChinese32x32(uint16_t x,uint16_t y,uint8_t *s,uint8_t sizey,uint16_t color)
{
	uint8_t i,j;
	uint16_t k;
	uint16_t HZnum;//Number of Chinese characters
	uint16_t TypefaceNum;//The byte size occupied by one character
	uint16_t x0=x;
	TypefaceNum=(sizey/8+((sizey%8)?1:0))*sizey;
	HZnum=sizeof(tfont32)/sizeof(typFNT_GB32);	//Count the number of Chinese characters
	for(k=0;k<HZnum;k++) 
	{
		if ((tfont32[k].Index[0]==*(s))&&(tfont32[k].Index[1]==*(s+1)))
		{ 	
			for(i=0;i<TypefaceNum;i++)
			{
				for(j=0;j<8;j++)
				{	
						if(tfont32[k].Msk[i]&(0x01<<j))	Paint_SetPixel(x,y,color);//Draw a dot
						x++;
						if((x-x0)==sizey)
						{
							x=x0;
							y++;
							break;
						}
				}
			}
		}				  	
		continue;   //If the corresponding dot matrix font library is found, exit immediately to prevent the impact of multiple Chinese characters being duplicated
	}
}


/*******************************************************************
Function Description: Display a single character
Interface description: x characters x coordinate parameters
Y character Y coordinate parameter
The characters to be displayed by chr
Size1 displays the font size of characters
Color Pixel Color Parameters
Return value: None
*******************************************************************/
void EPD_ShowChar(uint16_t x,uint16_t y,uint16_t chr,uint16_t size1,uint16_t color)
{
	uint16_t i,m,temp,size2,chr1;
	uint16_t x0,y0;
	x0=x,y0=y;
	if(size1==8)size2=6;
	else size2=(size1/8+((size1%8)?1:0))*(size1/2);  //Obtain the number of bytes occupied by the dot matrix set corresponding to one character in the font
	chr1=chr-' ';  //Calculate the offset value
	for(i=0;i<size2;i++)
	{
		if(size1==8)
			  {temp=asc2_0806[chr1][i];} //Call 0806 font
		else if(size1==12)
        {temp=asc2_1206[chr1][i];} //Call 1206 font
		else if(size1==16)
        {temp=asc2_1608[chr1][i];} //Call 1608 font
		else if(size1==24)
        {temp=asc2_2412[chr1][i];} //Call 2412 font
		else if(size1==48)
        {temp=asc2_4824[chr1][i];} //Call 4824 font 
		else return;
		for(m=0;m<8;m++)
		{
			if(temp&0x01)Paint_SetPixel(x,y,color);
			else Paint_SetPixel(x,y,!color);
			temp>>=1;
			y++;
		}
		x++;
		if((size1!=8)&&((x-x0)==size1/2))
		{x=x0;y0=y0+8;}
		y=y0;
  }
}

/*******************************************************************
		����˵������ʾ�ַ���
		�ӿ�˵����x 		 �ַ���x�������
              y 		 �ַ���Y�������
							*chr    Ҫ��ʾ���ַ���
              size1  ��ʾ�ַ����ֺŴ�С
              Color  ���ص���ɫ����
		����ֵ��  ��
*******************************************************************/
void EPD_ShowString(uint16_t x,uint16_t y,uint8_t *chr,uint16_t size1,uint16_t color)
{
	while(*chr!='\0')//�ж��ǲ��ǷǷ��ַ�!
	{
		EPD_ShowChar(x,y,*chr,size1,color);
		chr++;
		x+=size1/2;
  }
}
/*******************************************************************
		����˵����ָ������
		�ӿ�˵����m ����
              n ָ��
		����ֵ��  m��n�η�
*******************************************************************/
uint32_t EPD_Pow(uint16_t m,uint16_t n)
{
	uint32_t result=1;
	while(n--)
	{
	  result*=m;
	}
	return result;
}
/*******************************************************************
		����˵������ʾ��������
		�ӿ�˵����x 		 ����x�������
              y 		 ����Y�������
							num    Ҫ��ʾ������
              len    ���ֵ�λ��
              size1  ��ʾ�ַ����ֺŴ�С
              Color  ���ص���ɫ����
		����ֵ��  ��
*******************************************************************/
void EPD_ShowNum(uint16_t x,uint16_t y,uint32_t num,uint16_t len,uint16_t size1,uint16_t color)
{
	uint8_t t,temp,m=0;
	if(size1==8)m=2;
	for(t=0;t<len;t++)
	{
		temp=(num/EPD_Pow(10,len-t-1))%10;
			if(temp==0)
			{
				EPD_ShowChar(x+(size1/2+m)*t,y,'0',size1,color);
      }
			else 
			{
			  EPD_ShowChar(x+(size1/2+m)*t,y,temp+'0',size1,color);
			}
  }
}
/*******************************************************************
		����˵������ʾ����������
		�ӿ�˵����x 		 ����x�������
              y 		 ����Y�������
							num    Ҫ��ʾ�ĸ�����
              len    ���ֵ�λ��
              pre    �������ľ���
              size1  ��ʾ�ַ����ֺŴ�С
              Color  ���ص���ɫ����
		����ֵ��  ��
*******************************************************************/

void EPD_ShowFloatNum1(uint16_t x,uint16_t y,float num,uint8_t len,uint8_t pre,uint8_t sizey,uint8_t color)
{         	
	uint8_t t,temp,sizex;
	uint16_t num1;
	sizex=sizey/2;
	num1=num*EPD_Pow(10,pre);
	for(t=0;t<len;t++)
	{
		temp=(num1/EPD_Pow(10,len-t-1))%10;
		if(t==(len-pre))
		{
			EPD_ShowChar(x+(len-pre)*sizex,y,'.',sizey,color);
			t++;
			len+=1;
		}
	 	EPD_ShowChar(x+t*sizex,y,temp+48,sizey,color);
	}
}





//GUI��ʾ���
void EPD_ShowWatch(uint16_t x,uint16_t y,float num,uint8_t len,uint8_t pre,uint8_t sizey,uint8_t color)
{         	
	uint8_t t,temp,sizex;
	uint16_t num1;
	sizex=sizey/2;
	num1=num*EPD_Pow(10,pre);
	for(t=0;t<len;t++)
	{
		temp=(num1/EPD_Pow(10,len-t-1))%10;
		if(t==(len-pre))
		{
			EPD_ShowChar(x+(len-pre)*sizex+(sizex/2-2),y-6,':',sizey,color);
			t++;
			len+=1;
		}
	 	EPD_ShowChar(x+t*sizex,y,temp+48,sizey,color);
	}
}


void EPD_ShowPicture(uint16_t x,uint16_t y,uint16_t sizex,uint16_t sizey,const uint8_t BMP[],uint16_t Color)
{
	uint16_t j=0,t;
	uint16_t i,temp,y0,TypefaceNum=sizex*(sizey/8+((sizey%8)?1:0));
	y0=y;
  for(i=0;i<TypefaceNum;i++)
	{
		temp=BMP[j];
		for(t=0;t<8;t++)
		{
		 if(temp&0x80)
		 {
			 Paint_SetPixel(x,y,!Color);
		 }
		 else
		 {
			 Paint_SetPixel(x,y,Color);
		 }
		 y++;
		 temp<<=1;
		}
		if((y-y0)==sizey)
		{
			y=y0;
			x++;
		}
		j++;
	}
}


void Paint_ClearWindows(uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend, uint16_t Color)
{
    uint16_t X, Y;
    for (Y = Ystart; Y < Yend; Y++) {
        for (X = Xstart; X < Xend; X++) {//8 pixel =  1 byte
            Paint_SetPixel(X, Y, Color);
        }
    }
}


// 显示英文字符（直接数组查找）
void EPD_ShowEnglishChar(uint16_t x, uint16_t y, int char_index, uint16_t color) {
 //   if (char_index < 0 || char_index >= 54) return;  // 检查索引范围
    
    const uint8_t* char_data = english_font_16x16[char_index];
    uint16_t x0 = x;
    
    for(int i = 0; i < 16; i++) {
        uint8_t byte_data = char_data[i];
        for(int j = 0; j < 8; j++) {
            if(byte_data & (0x80 >> j)) {
                Paint_SetPixel(x, y, color);
            }
            x++;
            if((x - x0) == 16) {
                x = x0;
                y++;
                break;
            }
        }
    }
}

// 显示英文字符串示例
void EPD_ShowEnglishString(uint16_t x, uint16_t y, const int* char_indices, int count, uint16_t color) {
    uint16_t currentX = x;
    
    for(int i = 0; i < count; i++) {
        EPD_ShowEnglishChar(currentX, y, char_indices[i], color);
        currentX += 12; // 字符宽度 + 间距
        
        if(currentX + 16 > EPD_H) {
            currentX = x;
            y += 18; // 字符高度 + 间距
        }
    }
}