#pragma once
#include <cstring>
#include <cstdarg>

namespace ams::mitm::btdrv {

    void LogBase(const char *fmt, std::va_list args);

    inline void LogFmt(const char *fmt, ...) {
        std::va_list args;
        va_start(args, fmt);
        LogBase(fmt, args);
        va_end(args);

    }

    void LogData(void *data, size_t size);
}

#define BTDRV_LOG_FMT(fmt, ...) ::ams::mitm::btdrv::LogFmt(fmt "\n", ##__VA_ARGS__)
#define BTDRV_LOG_DATA(data, size) ::ams::mitm::btdrv::LogData(data, size)
