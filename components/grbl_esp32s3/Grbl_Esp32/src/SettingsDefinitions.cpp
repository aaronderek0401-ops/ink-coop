#include "Grbl.h"

FlagSetting* verbose_errors;

StringSetting* build_info;

IntSetting*   status_mask;

EnumSetting* message_level;

enum_opt_t messageLevels = {
    // clang-format off
    { "None", int8_t(MsgLevel::None) },
    { "Error", int8_t(MsgLevel::Error) },
    { "Warning", int8_t(MsgLevel::Warning) },
    { "Info", int8_t(MsgLevel::Info) },
    { "Debug", int8_t(MsgLevel::Debug) },
    { "Verbose", int8_t(MsgLevel::Verbose) },
    // clang-format on
};

void make_settings() {
    Setting::init();
    verbose_errors = new FlagSetting(EXTENDED, WG, NULL, "Errors/Verbose", DEFAULT_VERBOSE_ERRORS);

    build_info    = new StringSetting(EXTENDED, WG, NULL, "Firmware/Build", "");
    status_mask        = new IntSetting(GRBL, WG, "10", "Report/Status", DEFAULT_STATUS_REPORT_MASK, 0, 3);

    message_level = new EnumSetting(NULL, EXTENDED, WG, NULL, "Message/Level", static_cast<int8_t>(MsgLevel::Info), &messageLevels, NULL);
}
