#include "SPI_Init.h"
#include "EPD.h"
void EPD_SPIInit(void)
{
    // esp_err_t ret = 0;
    // /* ��ʼ��SPI���� */
    // EPD_GPIOInit();
    // /* SPI�����ӿ����� */
    // spi_device_interface_config_t devcfg = {
    //     .clock_speed_hz = 1 * 1000 * 1000,  /* SPIʱ�� */
    //     .mode = 0,                           /* SPIģʽ0 */
    //     .spics_io_num = BSP_SPI_CS_GPIO_PIN, /* SPI�豸���� */
    //     .queue_size = 7,                     /* ������гߴ� 7�� */
    // };
    // /* ����SPI�����豸 */
    // ret = spi_bus_add_device(SPI2_HOST, &devcfg, &EPD_Handle); /* ����SPI�����豸 */
    // ESP_ERROR_CHECK(ret);
}

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




