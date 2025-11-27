#include "ink_screen.h"
#include "./SPI_Init.h"

extern "C" {
	#include "./EPD.h"
#include "./EPD_GUI.h"
#include "./EPD_Font.h"
#include "./Pic.h"
}

uint8_t inkScreenTestFlag = 0; 
uint8_t inkScreenTestFlagTwo = 0;
const unsigned char *gamePeople[] = {game_1, game_2, game_3, game_4, game_5, game_6, game_7};
#define GAME_PEOPLE_COUNT (sizeof(gamePeople) / sizeof(gamePeople[0]))

const char *TAG = "ink_screen.cpp";
static TaskHandle_t _eventTaskHandle = NULL;
uint8_t ImageBW[12480];
WordEntry entry;

uint8_t *temp1111 = nullptr;
uint8_t *temp2222 = nullptr;

// 全局变量记录上次显示信息
static char lastDisplayedText[100] = {0};
static uint16_t lastTextLength = 0;
static uint16_t lastX = 65;
static uint16_t lastY = 120;

void showChinese16x16Str(uint16_t x, uint16_t y, uint8_t *s, uint8_t sizey, uint16_t color)
{
    uint16_t currentX = x;
    uint8_t* p = s;
    
    // 直接计算字符数（假设全是UTF-8中文，每个3字节）
    uint16_t char_count = strlen((char*)s) / 3;
    
    ESP_LOGI(TAG, "显示 %d 个汉字，屏幕高度: %d", char_count, EPD_H);
    
    for(uint16_t i = 0; i < char_count; i++)
    {
        // 检查是否超出屏幕高度
        if (y + sizey > EPD_W) {
            ESP_LOGW(TAG, "超出屏幕高度，停止显示。已显示 %d 个汉字", i);
            break;
        }
        
        // 显示单个汉字
        EPD_ShowChinese16x16_UTF8(currentX, y, p, sizey, color);
        
        // 移动到下一个字符
        p += 3;
        currentX += sizey + 3;
        
        // 换行检查
        if (currentX + sizey > EPD_H) {
            currentX = x;
            y += sizey + 3;
            ESP_LOGI(TAG,"#######换行");
            // 换行后再次检查高度
            if (y + sizey > EPD_W) {
                ESP_LOGW(TAG, "换行后超出屏幕高度，停止显示。已显示 %d 个汉字", i + 1);
                break;
            }
        }
    }
    
    ESP_LOGI(TAG, "显示完成");
}

void clearLastDisplay() {
    if (lastTextLength > 0) {
        // 计算需要清除的区域宽度（每个字符约8像素宽）
        uint16_t clearWidth = lastTextLength * 8;
        uint16_t clearHeight = 16; // 字体高度
        
        // 用白色矩形覆盖上次显示的区域
        EPD_DrawRectangle(lastX, lastY, lastX + clearWidth, lastY + clearHeight, WHITE,1);
        
        ESP_LOGI(TAG, "清除上次显示区域: 位置(%d,%d), 大小(%dx%d)", 
                 lastX, lastY, clearWidth, clearHeight);
    }
}

void updateDisplayWithClear(uint16_t x, uint16_t y, uint8_t *text, uint8_t size, uint16_t color) {
    // 清除上次显示的内容
    clearLastDisplay();
    
    // 显示新内容
    EPD_ShowString(x, y, text, size, color);
    
    // 更新记录
    strncpy(lastDisplayedText, (char*)text, sizeof(lastDisplayedText)-1);
    lastTextLength = strlen((char*)text);
    lastX = x;
    lastY = y;
    
    ESP_LOGI(TAG, "更新显示: 文本长度=%d, 内容=%s", lastTextLength, text);
}

int countLines(File &file) {
  int count = 0;
  file.seek(0);
  while (file.available()) {
    if (file.read() == '\n') count++;
  }
  file.seek(0);
  return count;
}

WordEntry readLineAtPosition(File &file, int lineNumber) {
  file.seek(0);
  int currentLine = 0;
  WordEntry entry;
  
  while (file.available() && currentLine <= lineNumber) {
    String line = file.readStringUntil('\n');
    if (currentLine == lineNumber) {
      parseCSVLine(line, entry);
      break;
    }
    currentLine++;
  }
  
  return entry;
}

void parseCSVLine(String line, WordEntry &entry) {
  int fieldCount = 0;
  String field = "";
  bool inQuotes = false;
  char lastChar = 0;
  
  for (int i = 0; i < line.length(); i++) {
    char c = line[i];
    
    if (lastChar != '\\' && c == '"') {
      inQuotes = !inQuotes;
    } else if (c == ',' && !inQuotes) {
      // 字段结束
      assignField(fieldCount, field, entry);
      field = "";
      fieldCount++;
    } else {
      field += c;
    }
    lastChar = c;
  }
  
  // 处理最后一个字段
  if (fieldCount < 5) {
    assignField(fieldCount, field, entry);
  }
}

void assignField(int fieldCount, String &field, WordEntry &entry) {
  // 移除字段两端的引号（如果存在）
  if (field.length() >= 2 && field[0] == '"' && field[field.length()-1] == '"') {
    field = field.substring(1, field.length()-1);
  }
  
  switch (fieldCount) {
    case 0: entry.word = field; break;
    case 1: entry.phonetic = field; break;
    case 2: entry.definition = field; break;
    case 3: entry.translation = field; break;
    case 4: entry.pos = field; break;
  }
}

void printWordEntry(WordEntry &entry, int lineNumber) {
  ESP_LOGI(TAG, "line number: %d", lineNumber);
  ESP_LOGI(TAG, "word: %s", entry.word.c_str());
  temp1111 = (uint8_t*) entry.word.c_str();
  if (entry.phonetic.length() > 0) {
    ESP_LOGI(TAG, "symbol: %s", entry.phonetic.c_str());
	temp2222 = (uint8_t*)entry.phonetic.c_str();
  }
  
  if (entry.definition.length() > 0) {
    ESP_LOGI(TAG, "English Definition: %s", entry.definition.c_str());
	//temp1111 = (uint8_t*)entry.definition.c_str();
  }
  
  if (entry.translation.length() > 0) {
    ESP_LOGI(TAG, "chinese Definition: %s", entry.translation.c_str());
  }
  
  if (entry.pos.length() > 0) {
    ESP_LOGI(TAG, "part of speech: %s", entry.pos.c_str());
  }
  
  ESP_LOGI(TAG, "----------------------------------------");
}

void readAndPrintWords() {
  File file = SD.open("/ecdict.mini.csv");
  if (!file) {
    ESP_LOGE(TAG, "无法打开CSV文件");
    return;
  }
  
  // 读取前几行作为示例
  ESP_LOGI(TAG, "显示前5个单词");
  int lineCount = 0;
  
  while (file.available() && lineCount < 5) {
    String line = file.readStringUntil('\n');
    if (line.length() > 0) {
      WordEntry entry;
      parseCSVLine(line, entry);
      printWordEntry(entry, lineCount);
      lineCount++;
    }
  }
  
  file.close();
  
  ESP_LOGI(TAG, "开始随机显示单词");
}

void readAndPrintRandomWord() {
  File file = SD.open("/ecdict.mini.csv");
  if (!file) {
    ESP_LOGE(TAG, "无法打开CSV文件");
    return;
  }
  
  // 计算总行数
  int totalLines = countLines(file);
  if (totalLines <= 1) {
    ESP_LOGE(TAG, "文件内容不足");
    file.close();
    return;
  }
  
  // 随机选择一行（跳过标题行）
  int randomLine = random(1, totalLines);
  entry = readLineAtPosition(file, randomLine);
  file.close();
  
  ESP_LOGI(TAG, "随机单词");
  printWordEntry(entry, randomLine);
}

void EPD_part_Show(uint16_t Xstart,uint16_t Ystart,uint16_t Xend,uint16_t Yend,uint16_t Color)
{
	EPD_PartInit();
	EPD_DrawLine(Xstart,Ystart,Xend,Yend,BLACK);
	EPD_Display(ImageBW);
	ESP_LOGI(TAG,"EPD_DrawLine");
	EPD_Update();
	vTaskDelay(100);
	EPD_DeepSleep();
}
void ink_screen_show(void *args)
{
    float num=12.05;
    uint8_t dat=0;
	 grbl_msg_sendf(CLIENT_SERIAL, MsgLevel::Info,"ink_screen_show");
	 Uart0.printf("ink_screen_show\r\n");
	static uint8_t cnt = 0;
	while(1)
	{
		switch(inkScreenTestFlag)
		{
			case 0:
				switch(inkScreenTestFlagTwo)
				{
					case 0:
					break;
					case 11:
					{
						EPD_FastInit();
						EPD_Display_Clear();
						EPD_Update();  //局刷之前先对E-Paper进行清屏操作
						EPD_PartInit();
						readAndPrintRandomWord();
						ESP_LOGI(TAG,"definition:%s",temp1111);
						//updateDisplayWithClear(50,50, (uint8_t*) entry.word.c_str(),24,BLACK);
						//updateDisplayWithClear(50,50, (uint8_t*)"A",16,BLACK);
						int phonetic_chars[] = {46}; // ɪ, ʌ, ʊ
    					EPD_ShowEnglishString(10, 40, phonetic_chars, 1, BLACK);
						// EPD_ShowEnglishChar(10, 40, 46, BLACK);
						//showChinese16x16Str(10,10,(uint8_t*)"哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦噢噢噢噢哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦哦",16,BLACK);
						//EPD_ShowChinese16x16(10,10,(uint8_t*)"的",16,BLACK);
						EPD_Display(ImageBW);
						EPD_Update();
						EPD_DeepSleep();
						inkScreenTestFlagTwo = 0;
					}
					break;
					case 22:
					break;
					case 33:
					break;
					case 44:
					break;
					case 55:
					 	EPD_FastInit();
						EPD_Display_Clear();
						EPD_Update();  //局刷之前先对E-Paper进行清屏操作
						EPD_PartInit();
						ESP_LOGI(TAG,"inkScreenTestFlagTwo\r\n");
						// 遍历所有图像
						for(int i = 0; i < GAME_PEOPLE_COUNT; i++) {
							EPD_ShowPicture(60,40,128,128,gamePeople[i],BLACK);
							EPD_Display(ImageBW);
							EPD_Update();
							vTaskDelay(1000);
							ESP_LOGI(TAG,"gamePeople[%d]\r\n",i);
						}
						EPD_DeepSleep();
						inkScreenTestFlagTwo = 0;

					break;
					case 66:
					break;

				}
			break;
			case 1:
				EPD_PartInit();
				EPD_ShowPicture(65,120,60,16,icon_7,BLACK);//显示黑色下划线
				// EPD_DrawRectangle(65,120,125,136,BLACK,1);
				EPD_Display(ImageBW);
				EPD_Update();
				EPD_DeepSleep();
				inkScreenTestFlag = 0;
			break;
			case 2:
				EPD_PartInit();
				//Paint_ClearWindows(65,120,65+60,120+16,WHITE);
				EPD_ShowPicture(65,120,60,16,icon_8,BLACK);//显示白色
				EPD_ShowPicture(185,120,60,16,icon_7,BLACK);//显示黑色下划线
				// EPD_DrawRectangle(185,120,245,136,BLACK,1);
				// EPD_DrawRectangle(65,120,125,136,WHITE,1);
				EPD_Display(ImageBW);
				EPD_Update();
				inkScreenTestFlag = 0;
				vTaskDelay(10);

				ESP_LOGI(TAG,"refresh is over\r\n");
				EPD_DeepSleep();
			break;
			case 3:
				EPD_PartInit();
				EPD_ShowPicture(185,120,60,16,icon_8,BLACK);//显示白色
				EPD_ShowPicture(305,120,60,16,icon_7,BLACK);//显示黑色下划线
				EPD_Display(ImageBW);
				EPD_Update();
				EPD_DeepSleep();
				inkScreenTestFlag = 0;
			break;
			case 4:
				EPD_PartInit();
				EPD_ShowPicture(305,120,60,16,icon_8,BLACK);//显示白色
				EPD_ShowPicture(65,200,60,16,icon_7,BLACK);//显示黑色下划线
				EPD_Display(ImageBW);
				EPD_Update();
				EPD_DeepSleep();
				inkScreenTestFlag = 0;
			break;
			case 5:
				EPD_PartInit();
				EPD_ShowPicture(65,200,60,16,icon_8,BLACK);//显示白色
				EPD_ShowPicture(185,200,60,16,icon_7,BLACK);//显示黑色
				EPD_Display(ImageBW);
				EPD_Update();
				EPD_DeepSleep();
				inkScreenTestFlag = 0;
			break;
			case 6:
				EPD_PartInit();
				EPD_ShowPicture(185,200,60,16,icon_8,BLACK);//显示白色
				EPD_ShowPicture(305,200,60,16,icon_7,BLACK);//显示黑色
				EPD_Display(ImageBW);
				EPD_Update();
				EPD_DeepSleep();
				inkScreenTestFlag = 0;
			break;
			case 7:
				EPD_FastInit();
				EPD_Display_Clear();
				EPD_Update();  //局刷之前先对E-Paper进行清屏操作
				delay_ms(100);
				EPD_PartInit();
				EPD_ShowPicture(60,40,62,64,icon_1,BLACK);
				EPD_ShowPicture(180,40,64,64,icon_2,BLACK);
				EPD_ShowPicture(300,40,86,64,icon_3,BLACK);
				EPD_ShowPicture(60,140,71,56,icon_4,BLACK);
				EPD_ShowPicture(180,140,76,56,icon_5,BLACK);
				EPD_ShowPicture(300,140,94,64,icon_6,BLACK);
				EPD_Display(ImageBW);
				EPD_Update();
				EPD_DeepSleep();
				inkScreenTestFlag = 0;
				vTaskDelay(1000);
			break;
		}
		// EPD_PartInit();
		// EPD_ShowPicture(24,0,368,200,gImage_3,BLACK);
		// EPD_DrawRectangle(20,200,55,235,BLACK,1);
		// EPD_DrawRectangle(80,200,115,235,BLACK,0); 
		// EPD_DrawCircle(331,220,18,BLACK,1); //Hollow circle.
		// EPD_DrawCircle(376,220,18,BLACK,0); 
		// EPD_ShowWatch(148,190,num,4,2,48,BLACK);
		// num+=0.01;
		// EPD_Display(ImageBW);
		// EPD_Update();
		// delay_ms(1000);
		// dat++;
		// if(dat==5)
		// {
		// 	EPD_FastInit();
		// 	while(1)
		// 	{
		// 	    EPD_Display_Clear();
		// 	    EPD_Update();
	    //         EPD_DeepSleep();
        //         vTaskDelay(10);
		// 	}
		// }
		//readAndPrintRandomWord();
        vTaskDelay(100);
	}
}
void ink_screen_init()
{
	 Uart0.printf("ink_screen_init\r\n");
	 
    EPD_GPIOInit();	
    Paint_NewImage(ImageBW,EPD_W,EPD_H,0,WHITE);//create  Canvas 
    Paint_Clear(WHITE);
	 Uart0.printf("Paint_Clear\r\n");
    /*********全刷********** */
    // EPD_Init();
	// 	 Uart0.printf("EPD_Init\r\n");
	// EPD_Display(gImage_1);
	// EPD_Update();
	// EPD_DeepSleep();
    // vTaskDelay(1000);   
	// Uart0.printf("all refresh\r\n");
    /************************快刷************************/
	EPD_FastInit();
	EPD_Display(iconAll_0);
	EPD_Update();
	EPD_DeepSleep();
	delay_ms(1000);
	Uart0.printf("fast refresh\r\n");
    /************************局刷************************/
 	// EPD_FastInit();
	// EPD_Display_Clear();
	// EPD_Update();  //局刷之前先对E-Paper进行清屏操作
	SDState state = get_sd_state(true);
        if (state != SDState::Idle) {
            if (state == SDState::NotPresent) {
				ESP_LOGI(TAG,"No SD Card\r\n");
            } else {
				ESP_LOGI(TAG,"SD Card Busy\r\n");
            }
        }
    BaseType_t task_created = xTaskCreatePinnedToCore(ink_screen_show, 
                                                        "ink_screen_show", 
                                                        4096, 
                                                        NULL, 
                                                        4, 
                                                        &_eventTaskHandle, 
                                                        0);
}