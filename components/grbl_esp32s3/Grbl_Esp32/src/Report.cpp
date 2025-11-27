/*
  Report.cpp - reporting and messaging methods
  Part of Grbl

  Copyright (c) 2012-2016 Sungeun K. Jeon for Gnea Research LLC

	2018 -	Bart Dring This file was modified for use on the ESP32
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
  along with Grbl.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
  This file functions as the primary feedback interface for Grbl. Any outgoing data, such
  as the protocol status messages, feedback messages, and status reports, are stored here.
  For the most part, these functions primarily are called from Protocol.cpp methods. If a
  different style feedback is desired (i.e. JSON), then a user can change these following
  methods to accommodate their needs.


	ESP32 Notes:

	Major rewrite to fix issues with BlueTooth. As described here there is a
	when you try to send data a single byte at a time using SerialBT.write(...).
	https://github.com/espressif/arduino-esp32/issues/1537

	A solution is to send messages as a string using SerialBT.print(...). Use
	a short delay after each send. Therefore this file needed to be rewritten
	to work that way. AVR Grbl was written to be super efficient to give it
	good performance. This is far less efficient, but the ESP32 can handle it.
	Do not use this version of the file with AVR Grbl.

	ESP32 discussion here ...  https://github.com/bdring/Grbl_Esp32/issues/3


*/

#include "Grbl.h"
#include <map>

#ifdef REPORT_HEAP
EspClass esp;
#endif
const int DEFAULTBUFFERSIZE = 64;

void grbl_send(uint8_t client, const char* text) {
    client_write(client, text);
}

// This is a formating version of the grbl_send(CLIENT_ALL,...) function that work like printf
void grbl_sendf(uint8_t client, const char* format, ...) {
    if (client == CLIENT_INPUT) {
        return;
    }
    char    loc_buf[64];
    char*   temp = loc_buf;
    va_list arg;
    va_list copy;
    va_start(arg, format);
    va_copy(copy, arg);
    size_t len = vsnprintf(NULL, 0, format, arg);
    va_end(copy);
    if (len >= sizeof(loc_buf)) {
        temp = new char[len + 1];
        if (temp == NULL) {
            return;
        }
    }
    len = vsnprintf(temp, len + 1, format, arg);
    grbl_send(client, temp);
    va_end(arg);
    if (temp != loc_buf) {
        delete[] temp;
    }
}
// Use to send [MSG:xxxx] Type messages. The level allows messages to be easily suppressed
void grbl_msg_sendf(uint8_t client, MsgLevel level, const char* format, ...) {
    if (client == CLIENT_INPUT) {
        return;
    }

    if (message_level != NULL) {  // might be null before messages are setup
        if (level > static_cast<MsgLevel>(message_level->get())) {
            return;
        }
    }

    char    loc_buf[100];
    char*   temp = loc_buf;
    va_list arg;
    va_list copy;
    va_start(arg, format);
    va_copy(copy, arg);
    size_t len = vsnprintf(NULL, 0, format, arg);
    va_end(copy);
    if (len >= sizeof(loc_buf)) {
        temp = new char[len + 1];
        if (temp == NULL) {
            return;
        }
    }
    len = vsnprintf(temp, len + 1, format, arg);
    grbl_sendf(client, "[MSG:%s]\r\n", temp);
    va_end(arg);
    if (temp != loc_buf) {
        delete[] temp;
    }
}

//function to notify
void grbl_notify(const char* title, const char* msg) {
#ifdef ENABLE_NOTIFICATIONS
    WebUI::notificationsservice.sendMSG(title, msg);
#endif
}

void grbl_notifyf(const char* title, const char* format, ...) {
    char    loc_buf[64];
    char*   temp = loc_buf;
    va_list arg;
    va_list copy;
    va_start(arg, format);
    va_copy(copy, arg);
    size_t len = vsnprintf(NULL, 0, format, arg);
    va_end(copy);
    if (len >= sizeof(loc_buf)) {
        temp = new char[len + 1];
        if (temp == NULL) {
            return;
        }
    }
    len = vsnprintf(temp, len + 1, format, arg);
    grbl_notify(title, temp);
    va_end(arg);
    if (temp != loc_buf) {
        delete[] temp;
    }
}

// Handles the primary confirmation protocol response for streaming interfaces and human-feedback.
// For every incoming line, this method responds with an 'ok' for a successful command or an
// 'error:'  to indicate some error event with the line or some critical system error during
// operation. Errors events can originate from the g-code parser, settings module, or asynchronously
// from a critical error, such as a triggered hard limit. Interface should always monitor for these
// responses.
void report_status_message(Error status_code, uint8_t client) {
    switch (status_code) {
        case Error::Ok:  // Error::Ok
#ifdef ENABLE_SD_CARD
            if (get_sd_state(true) == SDState::BusyPrinting) {
                SD_ready_next = true;  // flag so system_execute_line() will send the next line
            } else {
                grbl_send(client, "ok\r\n");
            }
#else
            grbl_send(client, "ok\r\n");
#endif
            break;
        default:
#ifdef ENABLE_SD_CARD
            // do we need to stop a running SD job?
            if (get_sd_state(true) == SDState::BusyPrinting) {
                if (status_code == Error::GcodeUnsupportedCommand) {
                    grbl_sendf(client, "error:%d\r\n", status_code);  // most senders seem to tolerate this error and keep on going
                    grbl_sendf(CLIENT_ALL, "error:%d in SD file at line %d\r\n", status_code, sd_get_current_line_number());
                    // don't close file
                    SD_ready_next = true;  // flag so system_execute_line() will send the next line
                } else {
                    grbl_notifyf("SD print error", "Error:%d during SD file at line: %d", status_code, sd_get_current_line_number());
                    grbl_sendf(CLIENT_ALL, "error:%d in SD file at line %d\r\n", status_code, sd_get_current_line_number());
                    closeFile();
                }
                return;
            }
#endif
            // With verbose errors, the message text is displayed instead of the number.
            // Grbl 0.9 used to display the text, while Grbl 1.1 switched to the number.
            // Many senders support both formats.
            if (verbose_errors->get()) {
                grbl_sendf(client, "error: %s\r\n", errorString(status_code));
            } else {
                grbl_sendf(client, "error:%d\r\n", static_cast<int>(status_code));
            }
    }
}

std::map<Message, const char*> MessageText = {
    { Message::CriticalEvent, "Reset to continue" },
    { Message::Enabled, "Enabled" },
    { Message::Disabled, "Disabled" },
    { Message::ProgramEnd, "Program End" },
    { Message::RestoreDefaults, "Restoring defaults" },
    { Message::SleepMode, "Sleeping" },
    // Handled separately due to numeric argument
    // { Message::SdFileQuit, "Reset during SD file at line: %d" },
};


// Welcome message
void report_init_message(uint8_t client) {
    grbl_sendf(client, "\r\nGrbl %s ['$' for help]\r\n", GRBL_VERSION);
}

// Prints build info line
void report_build_info(const char* line, uint8_t client) {
    grbl_sendf(client, "[VER:%s.%s:%s]\r\n[OPT:", GRBL_VERSION, GRBL_VERSION_BUILD, line);

#ifdef ENABLE_BLUETOOTH
    grbl_send(client, "B");
#endif
#ifdef ENABLE_SD_CARD
    grbl_send(client, "S");
#endif
#ifdef ENABLE_PARKING_OVERRIDE_CONTROL
    grbl_send(client, "R");
#endif
#if defined(ENABLE_WIFI)
    grbl_send(client, "W");
#endif
    // NOTE: Compiled values, like override increments/max/min values, may be added at some point later.
    // These will likely have a comma delimiter to separate them.
    grbl_send(client, "]\r\n");
    report_machine_type(client);
#if defined(ENABLE_WIFI)
    grbl_send(client, (char*)WebUI::wifi_config.info());
#endif
#if defined(ENABLE_BLUETOOTH)
    grbl_send(client, (char*)WebUI::bt_config.info());
#endif
}

// Calculate the position for status reports.
// float print_position = returned position
// float wco            = returns the work coordinate offset
// bool wpos            = true for work position compensation

// Prints real-time data. This function grabs a real-time snapshot of the stepper subprogram
// and the actual location of the CNC machine. Users may change the following function to their
// specific needs, but the desired real-time data report must be as short as possible. This is
// requires as it minimizes the computational overhead and allows grbl to keep running smoothly,
// especially during g-code programs with fast, short line segments and high frequency reports (5-20Hz).
void report_realtime_status(uint8_t client) {
    char status[200];
    char temp[20 * 20];

    strcpy(status, "<");
    strcat(status, report_state_text());


    // Returns planner and serial read buffer states.
#ifdef REPORT_FIELD_BUFFER_STATE
    if (bit_istrue(status_mask->get(), RtStatus::Buffer)) {
        int bufsize = DEFAULTBUFFERSIZE;
#    if defined(ENABLE_WIFI) && defined(ENABLE_TELNET)
        if (client == CLIENT_TELNET) {
            bufsize = WebUI::telnet_server.get_rx_buffer_available();
        }
#    endif  //ENABLE_WIFI && ENABLE_TELNET
#    if defined(ENABLE_BLUETOOTH)
        if (client == CLIENT_BT) {
            //TODO FIXME
            bufsize = 512 - WebUI::SerialBT.available();
        }
#    endif  //ENABLE_BLUETOOTH
        if (client == CLIENT_SERIAL) {
            bufsize = client_get_rx_buffer_available(CLIENT_SERIAL);
        }
        sprintf(temp, "|Bf:%d,%d", 100, bufsize);
        strcat(status, temp);
    }
#endif

#ifdef ENABLE_SD_CARD
    if (get_sd_state(true) == SDState::BusyPrinting) {
        sprintf(temp, "|SD:%4.2f,", sd_report_perc_complete());
        strcat(status, temp);
        sd_get_current_filename(temp);
        strcat(status, temp);
    }
#endif
#ifdef REPORT_HEAP
    sprintf(temp, "|Heap:%d", esp.getHeapSize());
    strcat(status, temp);
#endif
    strcat(status, ">\r\n");
    grbl_send(client, status);
}

void report_machine_type(uint8_t client) {
    grbl_msg_sendf(client, MsgLevel::Info, "Using machine:%s", MACHINE_NAME);
}

/*
    Print a message in hex format
    Ex: report_hex_msg(msg, "Rx:", 6);
    Would would print something like ... [MSG Rx: 0x01 0x03 0x01 0x08 0x31 0xbf]
*/
void report_hex_msg(char* buf, const char* prefix, int len) {
    char report[200];
    char temp[20];
    sprintf(report, "%s", prefix);
    for (int i = 0; i < len; i++) {
        sprintf(temp, " 0x%02X", buf[i]);
        strcat(report, temp);
    }

    grbl_msg_sendf(CLIENT_SERIAL, MsgLevel::Info, "%s", report);
}

void report_hex_msg(uint8_t* buf, const char* prefix, int len) {
    char report[200];
    char temp[20];
    sprintf(report, "%s", prefix);
    for (int i = 0; i < len; i++) {
        sprintf(temp, " 0x%02X", buf[i]);
        strcat(report, temp);
    }

    grbl_msg_sendf(CLIENT_SERIAL, MsgLevel::Info, "%s", report);
}

char* report_state_text() {
    static char state[10];

    switch (sys.state) {
        case State::Idle:
            strcpy(state, "Idle");
            break;
        case State::Alarm:
            strcpy(state, "Alarm");
            break;
        default:
            break;
    }
    return state;
}

void reportTaskStackSize(UBaseType_t& saved) {
#ifdef DEBUG_REPORT_STACK_FREE
    UBaseType_t newHighWater = uxTaskGetStackHighWaterMark(NULL);
    if (newHighWater != saved) {
        saved = newHighWater;
        grbl_msg_sendf(CLIENT_SERIAL, MsgLevel::Info, "%s Min Stack Space: %d", pcTaskGetTaskName(NULL), saved);
    }
#endif
}

