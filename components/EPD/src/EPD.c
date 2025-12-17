#include "EPD.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
uint8_t oldImage[12480];
/* SPI句柄 */
spi_device_handle_t EPD_Handle;

void EPD_GPIOInit(void)
{
    //   esp_err_t ret = 0;
    // spi_bus_config_t spi_bus_conf = {0};
    // /* SPI�������� */
    // spi_bus_conf.miso_io_num = -1;
    // spi_bus_conf.mosi_io_num = BSP_SPI_MOSI_GPIO_PIN; /* SPI_MOSI���� */
    // spi_bus_conf.sclk_io_num = BSP_SPI_CLK_GPIO_PIN;  /* SPI_SCLK���� */
    // spi_bus_conf.quadwp_io_num = -1;              /* SPIд�����ź����ţ�������δʹ�� */
    // spi_bus_conf.quadhd_io_num = -1;              /* SPI�����ź����ţ�������δʹ�� */
    // spi_bus_conf.max_transfer_sz = 416*240/4;     /* ����������С�����ֽ�Ϊ��λ */
   
	// /* ��ʼ��SPI���� */
    // ret = spi_bus_initialize(SPI3_HOST, &spi_bus_conf, SPI_DMA_CH_AUTO); /* SPI���߳�ʼ�� */
    // ESP_ERROR_CHECK(ret);   

	//     /* SPI驱动接口配置 */
    // spi_device_interface_config_t devcfg = {
    //     .clock_speed_hz = BSP_SPI_FREQ,  /* SPI时钟 */
    //     .mode = 0,                           /* SPI模式0 */
    //     .spics_io_num = BSP_SPI_CS_GPIO_PIN, /* SPI设备引脚 */
    //     .queue_size = 7,                     /* 事务队列尺寸 7个 */
    // };
    // /* 添加SPI总线设备 */
    // ret = spi_bus_add_device(SPI3_HOST, &devcfg, &EPD_Handle); /* 配置SPI总线设备 */
    // ESP_ERROR_CHECK(ret);

	//     gpio_config_t gpio_init_struct = {0};
    // gpio_init_struct.intr_type = GPIO_INTR_DISABLE;           /* 失能引脚中断 */
    // gpio_init_struct.mode = GPIO_MODE_OUTPUT;                 /* 输出模式 */
    // gpio_init_struct.pull_up_en = GPIO_PULLUP_ENABLE;         /* 使能上拉 */
    // gpio_init_struct.pull_down_en = GPIO_PULLDOWN_DISABLE;    /* 失能下拉 */
    // gpio_init_struct.pin_bit_mask = 1ull << EPD_RES_GPIO_PIN; /* 设置的引脚的位掩码 */
    // gpio_config(&gpio_init_struct);                           /* 配置GPIO */

    // gpio_init_struct.intr_type = GPIO_INTR_DISABLE;          /* 失能引脚中断 */
    // gpio_init_struct.mode = GPIO_MODE_OUTPUT;                /* 输出模式 */
    // gpio_init_struct.pull_up_en = GPIO_PULLUP_ENABLE;        /* 使能上拉 */
    // gpio_init_struct.pull_down_en = GPIO_PULLDOWN_DISABLE;   /* 失能下拉 */
    // gpio_init_struct.pin_bit_mask = 1ull << EPD_DC_GPIO_PIN; /* 设置的引脚的位掩码 */
    // gpio_config(&gpio_init_struct);                          /* 配置GPIO */

    // gpio_init_struct.intr_type = GPIO_INTR_DISABLE;            /* 失能引脚中断 */
    // gpio_init_struct.mode = GPIO_MODE_INPUT;                   /* 输入模式 */
    // gpio_init_struct.pull_up_en = GPIO_PULLUP_DISABLE;         /* 失能上拉 */
    // gpio_init_struct.pull_down_en = GPIO_PULLDOWN_ENABLE;     /* 使能下拉 */
    // gpio_init_struct.pin_bit_mask = 1ull << EPD_BUSY_GPIO_PIN; /* 设置的引脚的位掩码 */
    // gpio_config(&gpio_init_struct); 
}

/**
 * @brief       EPD读忙
 * @param       无
 * @retval      无
 * @note        BUSY 低电平为忙状态 高电平为空闲状态
 */
void EPD_READBUSY(void)
{
    while (1)
    {
        vTaskDelay(10/ portTICK_PERIOD_MS);
        if (EPD_ReadBUSY == 1)
        {
			ESP_LOGI("TAG","EPD_READBUSY");
            break;
        }
    }
}

/*******************************************************************
		函数说明:硬件复位函数
		入口参数:无
		说明:在E-Paper进入Deepsleep状态后需要硬件复位	
*******************************************************************/
void EPD_HW_RESET(void)
{
	vTaskDelay(100 / portTICK_PERIOD_MS);
	EPD_RES_Clr();
	vTaskDelay(20 / portTICK_PERIOD_MS);
	EPD_RES_Set();
	vTaskDelay(20 / portTICK_PERIOD_MS);
	EPD_READBUSY();
}

/*******************************************************************
		函数说明:更新函数
		入口参数:无	
		说明:更新显示内容到E-Paper		
*******************************************************************/
void EPD_Update(void)
{
  EPD_WR_REG(0x04);  //Power ON
	EPD_READBUSY();
	EPD_WR_REG(0x12);
	EPD_READBUSY();
}
/*******************************************************************
		函数说明:局刷初始化函数
		入口参数:无
		说明:E-Paper工作在局刷模式
*******************************************************************/
void EPD_PartInit(void)
{
	EPD_HW_RESET();
	EPD_READBUSY();
	EPD_WR_REG(0x00);
	EPD_WR_DATA8(EPD_Handle,0x1B,1);
	EPD_WR_REG(0xE0);
	EPD_WR_DATA8(EPD_Handle,0x02,1);
	EPD_WR_REG(0xE5);
	EPD_WR_DATA8(EPD_Handle,0x6E,1);
}
/*******************************************************************
		函数说明:快刷初始化函数
		入口参数:无
		说明:E-Paper工作在快刷模式
*******************************************************************/
void EPD_FastInit(void)
{
	EPD_HW_RESET();
	EPD_READBUSY();
	EPD_WR_REG(0x00);
	EPD_WR_DATA8(EPD_Handle,0x1B,1);
	EPD_WR_REG(0xE0);
	EPD_WR_DATA8(EPD_Handle,0x02,1);
	EPD_WR_REG(0xE5);
	EPD_WR_DATA8(EPD_Handle,0x5F,1);
}

/*******************************************************************
		函数说明:休眠函数
		入口参数:无
		说明:屏幕进入低功耗模式		
*******************************************************************/
void EPD_DeepSleep(void)
{
	EPD_WR_REG(0x02); //Power OFF
	EPD_READBUSY();
	EPD_WR_REG(0x07);
	EPD_WR_DATA8(EPD_Handle,0xA5,1);
}

/*******************************************************************
		函数说明:初始化函数
		入口参数:无
		说明:调整E-Paper默认显示方向
*******************************************************************/
void EPD_Init(void)
{
	EPD_HW_RESET();
	EPD_READBUSY();
	EPD_WR_REG(0x00);
	EPD_WR_DATA8(EPD_Handle,0x1B,1);
}

/*******************************************************************
		函数说明:清屏函数
		入口参数:无
		说明:E-Paper刷白屏
*******************************************************************/
void EPD_Display_Clear(void)
{
	uint16_t i,j,Width,Height;
	Width=(EPD_W%8==0)?(EPD_W/8):(EPD_W/8+1);
	Height=EPD_H;	
	EPD_WR_REG(0x10);
	for (j=0;j<Height;j++) 
	{
		for (i=0;i<Width;i++) 
		{
			EPD_WR_DATA8(EPD_Handle,oldImage[i+j*Width],1);
		}
	}
	EPD_WR_REG(0x13);
	for (j=0;j<Height;j++) 
	{
		for (i=0;i<Width;i++) 
		{
			EPD_WR_DATA8(EPD_Handle,0xFF,1);
			oldImage[i+j*Width]=0xFF;
		}
	}

}


/*******************************************************************
		函数说明:数组数据更新到E-Paper
		入口参数:无
		说明:
*******************************************************************/
void EPD_Display(const uint8_t *image)
{
	uint16_t i,j,Width,Height;
	Width=(EPD_W%8==0)?(EPD_W/8):(EPD_W/8+1);
	Height=EPD_H;
	EPD_WR_REG(0x10);
	for (j=0;j<Height;j++) 
	{
		for (i=0;i<Width;i++) 
		{
			EPD_WR_DATA8(EPD_Handle,oldImage[i+j*Width],1);
		}
	}
	EPD_WR_REG(0x13);
	for (j=0;j<Height;j++) 
	{
		for (i=0;i<Width;i++) 
		{
			EPD_WR_DATA8(EPD_Handle,image[i+j*Width],1);
			oldImage[i+j*Width]=image[i+j*Width];
		}
	}

}



