#ifndef _SPI_INIT_H_
#define _SPI_INIT_H_
#include <string.h>
#include "esp_log.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "../../grbl_esp32s3/Grbl_Esp32/src/Machines/inkScreen.h"
#define BSP_CS_Set() gpio_set_level(BSP_SPI_CS_GPIO_PIN,1)
#define BSP_CS_Clr() gpio_set_level(BSP_SPI_CS_GPIO_PIN,0)

#ifdef __cplusplus
extern "C" {
#endif

void EPD_WR_Bus(spi_device_handle_t handle, uint8_t dat);
void EPD_WR_REG(uint8_t reg);
void EPD_WR_DATA8(spi_device_handle_t handle, const uint8_t dat, int len);

#ifdef __cplusplus
}
#endif

#endif