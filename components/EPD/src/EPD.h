#ifndef _EPD_H_
#define _EPD_H_

#include "SPI_Init.h"

#define EPD_W	240 
#define EPD_H	416

#define WHITE 0xFF
#define BLACK 0x00

/* 定义管脚端口 */
#define EPD_RES_GPIO_PIN GPIO_NUM_12
#define EPD_DC_GPIO_PIN GPIO_NUM_13
#define EPD_BUSY_GPIO_PIN GPIO_NUM_4

/* 定义端口电平状态 */
#define EPD_RES_Set() gpio_set_level(EPD_RES_GPIO_PIN, 1)
#define EPD_RES_Clr() gpio_set_level(EPD_RES_GPIO_PIN, 0)

#define EPD_DC_Set() gpio_set_level(EPD_DC_GPIO_PIN, 1)
#define EPD_DC_Clr() gpio_set_level(EPD_DC_GPIO_PIN, 0)

#define EPD_ReadBUSY gpio_get_level(EPD_BUSY_GPIO_PIN)
extern spi_device_handle_t EPD_Handle;
void EPD_READBUSY(void);
void EPD_HW_RESET(void);
void EPD_Update(void);
void EPD_PartInit(void);
void EPD_FastInit(void);
void EPD_DeepSleep(void);
void EPD_Init(void);
void EPD_Display_Clear(void);
void EPD_Display(const uint8_t *image);
void EPD_GPIOInit(void);
#endif




