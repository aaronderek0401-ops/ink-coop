#include "SPI_Init.h"
#include "EPD.h"

void EPD_WR_Bus(spi_device_handle_t handle, uint8_t dat)
{
    esp_err_t ret;
    spi_transaction_t t = {0};
    t.length = 8;                                  /* Ҫ�����λ�� һ���ֽ� 8λ */
    t.tx_buffer = &dat;                            /* ����������ȥ */
    ret = spi_device_polling_transmit(handle, &t); /* ��ʼ���� */
    ESP_ERROR_CHECK(ret);
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
    ret = spi_device_polling_transmit(handle, &t); /* ��ʼ���� */
    ESP_ERROR_CHECK(ret);                          /* һ�㲻�������� */
}




