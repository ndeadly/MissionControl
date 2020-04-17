#pragma once

#include <cstdarg>
#include <cstdio>

namespace mc::log {

    void Write(const char *fmt, ...);
    void WriteData(void *data, size_t size);

}
