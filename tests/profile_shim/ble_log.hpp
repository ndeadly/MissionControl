#pragma once
#include <cstdio>
#include <string>
#include <vector>
namespace ams::ble::log {
    inline std::vector<std::string> messages;
    template<typename... Args> void Write(const char *format, Args... args) {
        char buffer[256];
        std::snprintf(buffer, sizeof(buffer), format, args...);
        messages.emplace_back(buffer);
    }
}
