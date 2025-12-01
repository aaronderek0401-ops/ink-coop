#ifndef _EPD_GUI_H_
#define _EPD_GUI_H_

#include "EPD.h"

typedef struct {
	uint8_t *Image;
	uint16_t width;
	uint16_t height;
	uint16_t widthMemory;
	uint16_t heightMemory;
	uint16_t color;
	uint16_t rotate;
	uint16_t widthByte;
	uint16_t heightByte;
	
}PAINT;
extern PAINT Paint;

//����E-Paper��ʾ���� 
/*******************
Rotaion:0-0�ȷ���
Rotaion:90-90�ȷ���
Rotaion:180-180�ȷ���
Rotaion:270-270�ȷ���
*******************/
#define Rotation 0  

#ifdef __cplusplus
extern "C" {
#endif
void Paint_NewImage(uint8_t *image,uint16_t Width,uint16_t Height,uint16_t Rotate,uint16_t Color); 					
void Paint_SetPixel(uint16_t Xpoint,uint16_t Ypoint,uint16_t Color);
void Paint_Clear(uint8_t Color);
void EPD_DrawLine(uint16_t Xstart,uint16_t Ystart,uint16_t Xend,uint16_t Yend,uint16_t Color);
void EPD_DrawRectangle(uint16_t Xstart,uint16_t Ystart,uint16_t Xend,uint16_t Yend,uint16_t Color,uint8_t mode);  
void EPD_DrawCircle(uint16_t X_Center,uint16_t Y_Center,uint16_t Radius,uint16_t Color,uint8_t mode);       
void EPD_ShowChar(uint16_t x,uint16_t y,uint16_t chr,uint16_t size1,uint16_t color);                       
void EPD_ShowString(uint16_t x,uint16_t y,uint8_t *chr,uint16_t size1,uint16_t color);                       
void EPD_ShowNum(uint16_t x,uint16_t y,uint32_t num,uint16_t len,uint16_t size1,uint16_t color);                 
void EPD_ShowPicture(uint16_t x,uint16_t y,uint16_t sizex,uint16_t sizey,const uint8_t BMP[],uint16_t Color);			     
void Paint_ClearWindows(uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend, uint16_t Color);
void EPD_ShowFloatNum1(uint16_t x,uint16_t y,float num,uint8_t len,uint8_t pre,uint8_t sizey,uint8_t color);
void EPD_ShowWatch(uint16_t x,uint16_t y,float num,uint8_t len,uint8_t pre,uint8_t sizey,uint8_t color);

/*************************************GB******************************************** */
void EPD_ShowChinese(uint16_t x,uint16_t y,uint8_t *s,uint8_t sizey,uint16_t color);
void EPD_ShowChinese12x12(uint16_t x,uint16_t y,uint8_t *s,uint8_t sizey,uint16_t color);
void EPD_ShowChinese16x16(uint16_t x,uint16_t y,uint8_t *s,uint8_t sizey,uint16_t color);
void EPD_ShowChinese24x24(uint16_t x,uint16_t y,uint8_t *s,uint8_t sizey,uint16_t color);
void EPD_ShowChinese32x32(uint16_t x,uint16_t y,uint8_t *s,uint8_t sizey,uint16_t color);
/*********************************************************************************** */

void EPD_ShowEnglishChar(uint16_t x, uint16_t y, int char_index, uint16_t color);//未用到
void EPD_ShowChar_24x24(uint16_t x, uint16_t y, char ch, uint16_t color);
void EPD_ShowChinese_UTF8_Single(uint16_t x, uint16_t y, uint8_t *s, uint8_t sizey, uint16_t color);
void EPD_ShowMixedString(uint16_t x, uint16_t y, uint8_t *str, uint8_t font_size, uint16_t color);
#ifdef __cplusplus
}
#endif

#endif



