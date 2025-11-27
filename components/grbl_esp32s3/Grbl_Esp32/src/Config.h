#pragma once
/*
Config.h - compile time configuration
Part of Grbl
Copyright (c) 2012-2016 Sungeun K. Jeon for Gnea Research LLC
Copyright (c) 2009-2011 Simen Svale Skogsrud
2018 -	Bart Dring This file was modifed for use on the ESP32
CPU. Do not use this with Grbl for atMega328P
Grbl is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
Grbl is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with Grbl.  If not, see http://www.gnu.org/licenses/.
*/

// This file contains compile-time configurations for Grbl's internal system. For the most part,
// users will not need to directly modify these, but they are here for specific needs, i.e.
// performance tuning or adjusting to non-typical machines.

// IMPORTANT: Any changes here requires a full re-compiling of the source code to propagate them.

/*
ESP 32 Notes
Some features should not be changed. See notes below.
*/

#include <Arduino.h>
#include "NutsBolts.h"

// It is no longer necessary to edit this file to choose
// a machine configuration; edit machine.h instead

// machine.h is #included below, after some definitions
// that the machine file might choose to undefine.

// Note: HOMING_CYCLES are now settings
#define SUPPORT_TASK_CORE 1  // Reference: CONFIG_ARDUINO_RUNNING_CORE = 1


// Include the file that loads the machine-specific config file.
// machine.h must be edited to choose the desired file.
#include "Machine.h"

// Serial baud rate
// OK to change, but the ESP32 boot text is 115200, so you will not see that is your
// serial monitor, sender, etc uses a different value than 115200
#define BAUD_RATE 115200

//Connect to your local AP with these credentials
//#define CONNECT_TO_SSID  "your SSID"
//#define SSID_PASSWORD  "your SSID password"L

//CONFIGURE_EYECATCH_BEGIN (DO NOT MODIFY THIS LINE)
// #define ENABLE_BLUETOOTH  // enable bluetooth
#define ENABLE_SD_CARD  // enable use of SD Card to run jobs
#define ENABLE_WIFI  //enable wifi
#define MACHINE_SERIAL_NUMBER

#if defined(ENABLE_WIFI) || defined(ENABLE_BLUETOOTH)
    #define WIFI_OR_BLUETOOTH
#endif

#define ENABLE_HTTP                //enable HTTP and all related services
// #define ENABLE_OTA                 //enable OTA
// #define ENABLE_TELNET              //enable telnet
// #define ENABLE_TELNET_WELCOME_MSG  //display welcome string when connect to telnet
// #define ENABLE_MDNS                //enable mDNS discovery
#define ENABLE_SSDP                //enable UPNP discovery
// #define ENABLE_NOTIFICATIONS       //enable notifications
#define ENABLE_SERIAL2SOCKET_IN
#define ENABLE_SERIAL2SOCKET_OUT


#ifdef ENABLE_AUTHENTICATION
    const char* const DEFAULT_ADMIN_PWD   = "admin";
    const char* const DEFAULT_USER_PWD    = "user";
    const char* const DEFAULT_ADMIN_LOGIN = "admin";
    const char* const DEFAULT_USER_LOGIN  = "user";
#endif

//Radio Mode
const int ESP_RADIO_OFF = 0;
const int ESP_WIFI_STA  = 1;
const int ESP_WIFI_AP   = 2;
const int ESP_BT        = 3;

//Default mode
#ifdef ENABLE_WIFI
    #ifdef CONNECT_TO_SSID
        const int DEFAULT_RADIO_MODE = ESP_WIFI_STA;
    #else
        const int DEFAULT_RADIO_MODE = ESP_WIFI_AP;
    #endif  //CONNECT_TO_SSID
#else
    #undef ENABLE_NOTIFICATIONS
    #ifdef ENABLE_BLUETOOTH
        const int DEFAULT_RADIO_MODE = ESP_BT;
    #else
        const int DEFAULT_RADIO_MODE = ESP_RADIO_OFF;
    #endif
#endif


enum class Cmd : uint8_t {
    Reset                 = 0x18,  // Ctrl-X
    StatusReport          = '?',
};

#define REPORT_FIELD_BUFFER_STATE        // Default enabled. Comment to disable.


// Time delay increments performed during a dwell. The default value is set at 50ms, which provides
// a maximum time delay of roughly 55 minutes, more than enough for most any application. Increasing
// this delay will increase the maximum dwell time linearly, but also reduces the responsiveness of
// run-time command executions, like status reports, since these are performed between each dwell
// time step. Also, keep in mind that the Arduino delay timer is not very accurate for long delays.
const int DWELL_TIME_STEP = 50;  // Integer (1-255) (milliseconds)

#define ENABLE_RESTORE_WIPE_ALL          // '$RST=*' Default enabled. Comment to disable.
#define ENABLE_RESTORE_DEFAULT_SETTINGS  // '$RST=$' Default enabled. Comment to disable.
#define ENABLE_RESTORE_CLEAR_PARAMETERS  // '$RST=#' Default enabled. Comment to disable.



