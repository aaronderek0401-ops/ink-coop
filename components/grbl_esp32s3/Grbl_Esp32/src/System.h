#pragma once

/*
  System.h - Header for system level commands and real-time processes
  Part of Grbl
  Copyright (c) 2014-2016 Sungeun K. Jeon for Gnea Research LLC

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
  along with Grbl.  If not, see <http://www.gnu.org/licenses/>.
*/

// Execution states and alarm
#include "Exec.h"

// System states. The state variable primarily tracks the individual functions
// of Grbl to manage each without overlapping. It is also used as a messaging flag for
// critical events.
enum class State : uint8_t {
    Idle = 0,    // Must be zero.
    Alarm,       // In alarm state. Locks out all g-code processes. Allows settings access.
};



// Global system variables
typedef struct {
    volatile State state;               // Tracks the current system state of Grbl.
    bool           abort;               // System abort flag. Forces exit back to main loop for reset.
} system_t;
extern system_t sys;

void system_ini();  // Renamed from system_init() due to conflict with esp32 files
// Execute the startup script lines stored in non-volatile storage upon initialization
Error execute_line(char* line, uint8_t client, WebUI::AuthenticationLevel auth_level);
Error system_execute_line(char* line, WebUI::ESPResponseStream*, WebUI::AuthenticationLevel);
Error system_execute_line(char* line, uint8_t client, WebUI::AuthenticationLevel);
Error do_command_or_setting(const char* key, char* value, WebUI::AuthenticationLevel auth_level, WebUI::ESPResponseStream*);

