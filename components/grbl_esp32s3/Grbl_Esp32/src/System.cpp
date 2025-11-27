/*
  System.cpp - Header for system level commands and real-time processes
  Part of Grbl
  Copyright (c) 2014-2016 Sungeun K. Jeon for Gnea Research LLC

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

#include "Grbl.h"
#include "Config.h"

// Declare system global variable structure
system_t               sys;
xQueueHandle control_sw_queue;    // used by control switch debouncing
bool         debouncing = false;  // debouncing in process

void system_ini() {  // Renamed from system_init() due to conflict with esp32 files
    // setup control inputs
    //customize pin definition if needed
#if (GRBL_SPI_SS != -1) || (GRBL_SPI_MISO != -1) || (GRBL_SPI_MOSI != -1) || (GRBL_SPI_SCK != -1)
    SPI.begin(GRBL_SPI_SCK, GRBL_SPI_MISO, GRBL_SPI_MOSI, GRBL_SPI_SS);
#endif

}
