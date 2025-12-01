#pragma once



// ===================== pin definiions ==============================
#define MACHINE_NAME "BLGS"
#define VERSION "251124"
#define DEFAULT_VERBOSE_ERRORS 1
#define MACHINE_TYPE "BLGS"
#define  WAVELENGTH 13
#define POWER 5
#define HTTP_PORT_8848

// comunications
#define PC_UART_TX_PIN              TXD0   // TXD0, TX in pins_arduino.h
#define PC_UART_RX_PIN              RXD0   // RXD0, RX in pins_arduino.h

// // SD card
#define ENABLE_SD_CARD
#define SDCARD_DET_PIN              GPIO_NUM_11
#define GRBL_SPI_SS                 GPIO_NUM_36
#define GRBL_SPI_MOSI               GPIO_NUM_38
#define GRBL_SPI_MISO               GPIO_NUM_39
#define GRBL_SPI_SCK                GPIO_NUM_37
#define GRBL_SPI_FREQ                40 * 1000 * 1000

/* 墨水屏 */
#define EPD_RES_GPIO_PIN            GPIO_NUM_12
#define EPD_DC_GPIO_PIN             GPIO_NUM_13
#define EPD_BUSY_GPIO_PIN           GPIO_NUM_4
#define BSP_SPI_MOSI_GPIO_PIN       GPIO_NUM_47
#define BSP_SPI_CLK_GPIO_PIN        GPIO_NUM_48
#define BSP_SPI_CS_GPIO_PIN         GPIO_NUM_14
#define BSP_SPI_FREQ                20 * 1000 * 1000

#define DEFAULT_X_MAX_TRAVEL 450.0
#define DEFAULT_Y_MAX_TRAVEL 440.0
