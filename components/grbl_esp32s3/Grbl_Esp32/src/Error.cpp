/*
  Error.cpp - Error names
  Part of Grbl
  Copyright (c) 2014-2016 Sungeun K. Jeon for Gnea Research LLC

	2018 -	Bart Dring This file was modifed for use on the ESP32
					CPU. Do not use this with Grbl for atMega328P
        2020 - Mitch Bradley

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

#include "Error.h"

std::map<Error, const char*> ErrorNames = {
    { Error::Ok, "No error" },
    { Error::BadNumberFormat, "Bad GCode number format" },
    { Error::InvalidStatement, "Invalid $ statement" },
    { Error::NegativeValue, "Negative value" },
    { Error::SettingDisabled, "Setting disabled" },
    { Error::SettingReadFail, "Failed to read settings" },
    { Error::IdleError, "Command requires idle state" },
    { Error::SystemGcLock, "System locked (alarm state)" },
    { Error::Overflow, "Buffer overflow" },
    { Error::Eol, "End of line" },
    { Error::FsFailedMount, "Failed to mount device" },
    { Error::FsFailedRead, "Failed to read" },
    { Error::FsFailedOpenDir, "Failed to open directory" },
    { Error::FsDirNotFound, "Directory not found" },
    { Error::FsFileEmpty, "File empty" },
    { Error::FsFileNotFound, "File not found" },
    { Error::FsFailedOpenFile, "Failed to open file" },
    { Error::FsFailedBusy, "Device is busy" },
    { Error::FsFailedDelDir, "Failed to delete directory" },
    { Error::FsFailedDelFile, "Failed to delete file" },
    { Error::BtFailBegin, "Bluetooth failed to start" },
    { Error::WifiFailBegin, "WiFi failed to start" },
    { Error::NumberRange, "Number out of range for setting" },
    { Error::InvalidValue, "Invalid value for setting" },
    { Error::MessageFailed, "Failed to send message" },
    { Error::NvsSetFailed, "Failed to store setting" },
    { Error::NvsGetStatsFailed, "Failed to get setting status" },
    { Error::AnotherInterfaceBusy, "Another interface is busy" },
};
