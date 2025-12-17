#include "SPI_Init.h"
#include "EPD.h"

void EPD_WR_Bus(spi_device_handle_t handle, uint8_t dat)
{
    esp_err_t ret;
    spi_transaction_t t = {0};
    t.length = 8;                                  /* Ҫ�����λ�� һ���ֽ� 8λ */
    t.tx_buffer = &dat;                            /* ����������ȥ */
    if (handle == NULL) {
        ESP_LOGW("SPI_Init", "EPD_WR_Bus called with NULL handle, skipping transmit");
        return;
    }
    ret = spi_device_polling_transmit(handle, &t); /* ��ʼ���� */
    if (ret != ESP_OK) {
        ESP_LOGE("SPI_Init", "spi_device_polling_transmit failed: %d", ret);
    }
}

void EPD_WR_REG(uint8_t reg)
{
	EPD_DC_Clr();
	EPD_WR_Bus(EPD_Handle,reg);
	EPD_DC_Set();
}
void EPD_WR_DATA8(spi_device_handle_t handle, const uint8_t dat, int len)
{
    esp_err_t ret;
    spi_transaction_t t = {0};
    if (len == 0)
    {
        return; /* ����Ϊ0 û������Ҫ���� */
    }
    t.length = len * 8;                            /* Ҫ�����λ�� һ���ֽ� 8λ */
    t.tx_buffer = &dat;                             /* ����������ȥ */
    if (handle == NULL) {
        ESP_LOGW("SPI_Init", "EPD_WR_DATA8 called with NULL handle, skipping transmit");
        return;
    }
    ret = spi_device_polling_transmit(handle, &t); /* ��ʼ���� */
    if (ret != ESP_OK) {
        ESP_LOGE("SPI_Init", "spi_device_polling_transmit failed: %d", ret);
    }
}




