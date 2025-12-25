# Always compiled source files
set(srcs Grbl_Esp32/src/Grbl.cpp
    Grbl_Esp32/src/Error.cpp
    Grbl_Esp32/src/Exec.cpp
    Grbl_Esp32/src/ProcessSettings.cpp
    Grbl_Esp32/src/Protocol.cpp
    Grbl_Esp32/src/Regex.cpp
    Grbl_Esp32/src/Report.cpp
    Grbl_Esp32/src/SDCard.cpp
    Grbl_Esp32/src/Serial.cpp
    Grbl_Esp32/src/Settings.cpp
    Grbl_Esp32/src/SettingsDefinitions.cpp
    Grbl_Esp32/src/System.cpp
    Grbl_Esp32/src/Uart.cpp
    Grbl_Esp32/src/WebUI/Authentication.cpp
    Grbl_Esp32/src/WebUI/BTConfig.cpp
    Grbl_Esp32/src/WebUI/Commands.cpp
    Grbl_Esp32/src/WebUI/ESPResponse.cpp
    Grbl_Esp32/src/WebUI/InputBuffer.cpp
    Grbl_Esp32/src/WebUI/JSONEncoder.cpp
    Grbl_Esp32/src/WebUI/NotificationsService.cpp
    Grbl_Esp32/src/WebUI/Serial2Socket.cpp
    Grbl_Esp32/src/WebUI/TelnetServer.cpp
    Grbl_Esp32/src/WebUI/WebServer.cpp
    Grbl_Esp32/src/WebUI/WebSettings.cpp
    Grbl_Esp32/src/WebUI/WifiConfig.cpp
    Grbl_Esp32/src/WebUI/WifiServices.cpp
    Grbl_Esp32/src/BL_add/ink_screen/ink_screen.cpp
    Grbl_Esp32/src/BL_add/ink_screen/web_layout.cpp
    Grbl_Esp32/src/BL_add/ink_screen/sd_font_loader.cpp
    Grbl_Esp32/src/BL_add/ink_screen/chinese_text_display.cpp
    Grbl_Esp32/src/BL_add/ink_screen/image_loader.cpp
    Grbl_Esp32/src/BL_add/ota/ota_update.cpp
    Grbl_Esp32/src/BL_add/ink_screen/word_book.cpp
    Grbl_Esp32/src/BL_add/ink_screen/wordbook_interface.cpp
    Grbl_Esp32/src/BL_add/ink_screen/sleep_interface.cpp
    Grbl_Esp32/src/BL_add/ink_screen/ttf_font_manager.cpp
    Grbl_Esp32/src/BL_add/ink_screen/multi_font_manager.cpp
     )

# Always included headers
set(includes Grbl_Esp32 
             Grbl_Esp32/src/BL_add/ink_screen)

idf_component_register(SRCS ${srcs}
                       INCLUDE_DIRS ${includes}
                       REQUIRES soc driver hal arduino_esp32 ESP32SSDP arduinoWebSockets newlib
                       esp_http_server EPD GXEPD2 AdafruitGFX)
