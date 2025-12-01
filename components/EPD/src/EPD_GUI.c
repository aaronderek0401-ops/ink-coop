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
	HZnum=sizeof(tfont16)/sizeof(typFONT_GB16);	//Count the number of Chinese characters
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

void EPD_ShowChinese_UTF8_Single(uint16_t x, uint16_t y, uint8_t *s, uint8_t font_size, uint16_t color)
{
    uint8_t i,j;
    uint16_t k;
    uint16_t HZnum;
    uint16_t TypefaceNum;
    uint16_t x0 = x;
    
    TypefaceNum = (font_size/8 + ((font_size%8)?1:0)) * font_size;
    HZnum = sizeof(tfontUTF8_16)/sizeof(typFNT_UTF8_16);
    
    // 检查是否是UTF-8中文字符
    if ((s[0] & 0xE0) != 0xE0) {
        ESP_LOGE("TAG", "不是UTF-8中文字符: 0x%02X", s[0]);
        return;
    }
    
  //  ESP_LOGI("TAG", "查找UTF-8汉字: 0x%02X%02X%02X", s[0], s[1], s[2]);
    
    for(k = 0; k < HZnum; k++) 
    {
        // UTF-8编码比较（3个字节）
        if (tfontUTF8_16[k].Index[0] == s[0] &&
            tfontUTF8_16[k].Index[1] == s[1] &&
            tfontUTF8_16[k].Index[2] == s[2])
        { 	
          //  ESP_LOGI("TAG", "找到汉字: %s", tfontUTF8_16[k].Index);
            
            for(i = 0; i < TypefaceNum; i++)
            {
                for(j = 0; j < 8; j++)
                {	
                    if(tfontUTF8_16[k].Msk[i] & (0x01 << j)) {
                        Paint_SetPixel(x, y, color);
                    }
                    x++;
                    if((x - x0) == font_size)
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
// 显示占位符（用于调试）
void drawPlaceholder(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color) {
    // 绘制边框
    for(uint16_t i = x; i < x + width; i++) {
        Paint_SetPixel(i, y, color);
        Paint_SetPixel(i, y + height - 1, color);
    }
    for(uint16_t j = y; j < y + height; j++) {
        Paint_SetPixel(x, j, color);
        Paint_SetPixel(x + width - 1, j, color);
    }
    
    // 绘制对角线
    for(uint16_t i = 0; i < width && i < height; i++) {
        Paint_SetPixel(x + i, y + i, color);
    }
}

#define FONT_TABLE_SIZE (sizeof(fontTableChar_24X24) / sizeof(font_entry_t))

// 查找字符对应的字体数据
const font_entry_t* find_font_entry(char ch) {
    for (int i = 0; i < FONT_TABLE_SIZE; i++) {
        if (fontTableChar_24X24[i].ch[0] == ch) {
            return &fontTableChar_24X24[i];
        }
    }
    return NULL; // 未找到该字符
}

// 检查字符是否在字体库中
bool isChineseCharInFont(uint8_t *chinese_char, uint8_t font_size) {
    // 这里可以根据你的字体库实现具体的检查逻辑
    // 暂时返回true，假设所有字符都在字体库中
    return true;
}
#include <ctype.h> 

void handleASCIIChar(char c, uint16_t *x, uint16_t y, uint8_t font_size, uint16_t color, uint8_t english_width) {
    if (c < 32) return; // 跳过控制字符
    
    // 标点符号和空格处理
    switch (c) {
        case ' ':
            *x += english_width / 3; // 空格更窄
            break;
        case '.':
        case ',':
        case ';':
        case ':':
        case '!':
        case '?':
            EPD_ShowChar(*x, y, c, font_size, color);
            *x += english_width / 2; // 标点符号宽度减半
            break;
        case '(':
        case ')':
        case '[':
        case ']':
        case '{':
        case '}':
            EPD_ShowChar(*x, y, c, font_size, color);
            *x += english_width * 2 / 3; // 括号中等宽度
            break;
        default:
            EPD_ShowChar(*x, y, c, font_size, color);
            *x += english_width; // 普通字符正常宽度
            break;
    }
}
// 增强的中文标点检测
bool isChinesePunctuation(uint8_t *chinese_char) {
    // 完整的中文标点符号列表
    static const char* chinese_punctuations[] = {
        "\xEF\xBC\x8C", // ，
        "\xEF\xBC\x8E", // ．
        "\xEF\xBC\x9A", // ：
        "\xEF\xBC\x9B", // ；
        "\xEF\xBC\x9F", // ？
        "\xEF\xBC\x81", // ！
        "\xEF\xBC\x88", // （
        "\xEF\xBC\x89", // ）
        "\xE3\x80\x80", // 全角空格
        "\xE3\x80\x81", // 、
        "\xE3\x80\x82", // 。
        "\xE3\x80\x90", // 【
        "\xE3\x80\x91", // 】
        NULL
    };
    
    for (int i = 0; chinese_punctuations[i] != NULL; i++) {
        if (memcmp(chinese_char, chinese_punctuations[i], 3) == 0) {
            return true;
        }
    }
    return false;
}

void EPD_ShowMixedString(uint16_t x, uint16_t y, uint8_t *str, uint8_t font_size, uint16_t color) {
    if (str == NULL || strlen((char*)str) == 0) {
        ESP_LOGE("TAG", "输入字符串为空");
        return;
    }
    
    uint16_t current_x = x;
    uint16_t current_y = y;
    uint8_t chinese_width = 16;  // 中文字符固定宽度
    uint8_t chinese_height = 16; // 中文字符固定高度
    uint8_t english_width = 8;   // 英文字符宽度
    
    ESP_LOGI("TAG", "显示混合字符串: %s", str);
    
    int i = 0;
    while (str[i] != '\0') {
        // 处理换行符
        if (str[i] == '\n') {
            current_x = x;
            current_y += chinese_height + 2;
            i++;
            continue;
        }
        
        // 判断字符类型
        if ((str[i] & 0x80) == 0) {
            // ASCII字符
            if (str[i] >= 32 && str[i] <= 126) { // 可打印字符
                // 特殊字符处理
                if (str[i] == ' ') {
                    current_x += english_width / 2;
                } else if (str[i] == '.' || str[i] == ',' || str[i] == ';' || str[i] == ':') {
                    // 标点符号
                    EPD_ShowChar(current_x, current_y, str[i], font_size, color);
                    current_x += english_width / 2;
                } else {
                    // 普通英文字符
                    EPD_ShowChar(current_x, current_y, str[i], font_size, color);
                    current_x += english_width;
                }
            }
            i++;
        }
        else if ((str[i] & 0xE0) == 0xE0) {
            // 中文字符 (3字节UTF-8)
            if (str[i+1] != '\0' && str[i+2] != '\0') {
                uint8_t chinese_char[4] = {str[i], str[i+1], str[i+2], '\0'};
                
                 EPD_ShowChinese_UTF8_Single(current_x, current_y, chinese_char, 16, color);
                current_x += chinese_width;
                
                ESP_LOGD("TAG", "显示中文字符: '%s' at (%d,%d)", chinese_char, current_x - chinese_width, current_y);
                i += 3;
            } else {
                i++; // 不完整的UTF-8序列
            }
        }
        else {
            // 其他UTF-8字符，跳过
            if ((str[i] & 0xE0) == 0xC0) i += 2;
            else if ((str[i] & 0xF0) == 0xF0) i += 4;
            else i++;
        }
        
        // 换行处理
        if (current_x > 400 - chinese_width) {
            current_x = x;
            current_y += chinese_height + 2;
        }
        
        // 安全限制
        if (i > 200) break;
    }
    
    ESP_LOGI("TAG", "字符串显示完成");
}
void EPD_ShowChar_24x24(uint16_t x, uint16_t y, char ch, uint16_t color)
{
    const font_entry_t* font_entry = find_font_entry(ch);
    if (font_entry == NULL) {
        ESP_LOGE("TAG", "未找到字符 '%c' 的字体数据", ch);
        return;
    }

    uint8_t i, j;
    uint16_t x0 = x;
    uint16_t y0 = y;
    
    // 24x24字体参数
    uint8_t char_width = 24;
    uint8_t char_height = 24;
    uint8_t bytes_per_row = 3;  // 24x24: 每行3字节 (24位)
    
    const uint8_t* bitmap = font_entry->bits;
    
    ESP_LOGI("TAG", "显示字符 '%c' 在位置 (%d, %d)", ch, x, y);
    
    // 显示点阵数据
    for(i = 0; i < char_height; i++)  // 行循环
    {
        x = x0;  // 每行开始时重置x坐标
        
        for(j = 0; j < char_width; j++)  // 列循环
        {
            // 计算字节索引和位索引
            uint8_t byte_index = i * bytes_per_row + j / 8;
            uint8_t bit_index = j % 8;
            
            if(byte_index < 72) {
                // 检查该位是否置1（从高位到低位）
                if(bitmap[byte_index] & (0x80 >> bit_index)) {
                    Paint_SetPixel(x, y, color);
                }
            }
            x++;
        }
        y++;  // 下一行
    }
}