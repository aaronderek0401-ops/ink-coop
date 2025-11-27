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
#define BAUD_RATE 921600

//Connect to your local AP with these credentials
//#define CONNECT_TO_SSID  "your SSID"
//#define SSID_PASSWORD  "your SSID password"

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

// Captive portal is used when WiFi is in access point mode.  It lets the
// WebUI come up automatically in the browser, instead of requiring the user
// to browse manually to a default URL.  It works like airport and hotel
// WiFi that takes you a special page as soon as you connect to that AP.
// #define ENABLE_CAPTIVE_PORTAL

// Warning! The current authentication implementation is too weak to provide
// security against an attacker, since passwords are stored and transmitted
// "in the clear" over unsecured channels.  It should be treated as a
// "friendly suggestion" to prevent unwitting dangerous actions, rather than
// as effective security against malice.
// #define ENABLE_AUTHENTICATION
//CONFIGURE_EYECATCH_END (DO NOT MODIFY THIS LINE)

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


// The status report change for Grbl v1.1 and after also removed the ability to disable/enable most data
// fields from the report. This caused issues for GUI developers, who've had to manage several scenarios
// and configurations. The increased efficiency of the new reporting style allows for all data fields to
// be sent without potential performance issues.
// NOTE: The options below are here only provide a way to disable certain data fields if a unique
// situation demands it, but be aware GUIs may depend on this data. If disabled, it may not be compatible.
#define REPORT_FIELD_BUFFER_STATE        // Default enabled. Comment to disable.

// Time delay increments performed during a dwell. The default value is set at 50ms, which provides
// a maximum time delay of roughly 55 minutes, more than enough for most any application. Increasing
// this delay will increase the maximum dwell time linearly, but also reduces the responsiveness of
// run-time command executions, like status reports, since these are performed between each dwell
// time step. Also, keep in mind that the Arduino delay timer is not very accurate for long delays.
const int DWELL_TIME_STEP = 50;  // Integer (1-255) (milliseconds)



// Adjusts homing cycle search and locate scalars. These are the multipliers used by Grbl's
// homing cycle to ensure the limit switches are engaged and cleared through each phase of
// the cycle. The search phase uses the axes max-travel setting times the SEARCH_SCALAR to
// determine distance to look for the limit switch. Once found, the locate phase begins and
// uses the homing pull-off distance setting times the LOCATE_SCALAR to pull-off and re-engage
// the limit switch.
// NOTE: Both of these values must be greater than 1.0 to ensure proper function.
// #define HOMING_AXIS_SEARCH_SCALAR  1.5 // Uncomment to override defaults in limits.c.
// #define HOMING_AXIS_LOCATE_SCALAR  10.0 // Uncomment to override defaults in limits.c.

// Enable the '$RST=*', '$RST=$', and '$RST=#' eeprom restore commands. There are cases where
// these commands may be undesirable. Simply comment the desired macro to disable it.
#define ENABLE_RESTORE_WIPE_ALL          // '$RST=*' Default enabled. Comment to disable.
#define ENABLE_RESTORE_DEFAULT_SETTINGS  // '$RST=$' Default enabled. Comment to disable.
#define ENABLE_RESTORE_CLEAR_PARAMETERS  // '$RST=#' Default enabled. Comment to disable.


