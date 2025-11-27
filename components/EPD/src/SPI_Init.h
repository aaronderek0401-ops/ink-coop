#ifndef _SPI_INIT_H_
#define _SPI_INIT_H_
#include <string.h>
#include "esp_log.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"

#define BSP_SPI_MOSI_GPIO_PIN GPIO_NUM_47
#define BSP_SPI_CLK_GPIO_PIN GPIO_NUM_48
#define BSP_SPI_CS_GPIO_PIN GPIO_NUM_14

/* ����˿ڵ�ƽ״̬ */
#define BSP_CS_Set() gpio_set_level(BSP_SPI_CS_GPIO_PIN,1)
#define BSP_CS_Clr() gpio_set_level(BSP_SPI_CS_GPIO_PIN,0)

void EPD_WR_Bus(spi_device_handle_t handle, uint8_t dat);
void EPD_WR_REG(uint8_t reg);	//д��ָ��
void EPD_WR_DATA8(spi_device_handle_t handle, const uint8_t dat, int len);


#endif