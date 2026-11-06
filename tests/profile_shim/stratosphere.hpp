#pragma once
#include <cassert>
#include <memory>
#include <mutex>
#include <string>
#include <cstring>

struct Result {
    unsigned value = 0;
    unsigned GetValue() const { return value; }
};
#define R_FAILED(r) ((r).value != 0)

template<class F> struct ScopeExit {
    F function;
    ~ScopeExit() { function(); }
};
struct ScopeExitTag {};
template<class F> auto operator+(ScopeExitTag, F function) { return ScopeExit<F>{function}; }
#define ON_SCOPE_EXIT auto scope_exit = ScopeExitTag{} + [&]()

namespace ams::os {
    using SdkMutex = std::mutex;
    inline s64 now_us = 1000;
    struct TimeSpan {
        s64 microseconds;
        s64 GetMicroSeconds() const { return microseconds; }
        static TimeSpan FromSeconds(s64 seconds) { return {seconds * 1000000}; }
        static TimeSpan FromMilliSeconds(s64 ms) { return {ms * 1000}; }
        TimeSpan operator+(TimeSpan other) const { return {microseconds + other.microseconds}; }
        bool operator>=(TimeSpan other) const { return microseconds >= other.microseconds; }
    };
    struct Tick {
        s64 value;
        explicit Tick(s64 tick) : value(tick) {}
        s64 GetInt64Value() const { return value; }
        TimeSpan ToTimeSpan() const { return {value}; }
    };
    inline Tick GetSystemTick() { return Tick(now_us); }
    inline void (*sleep_hook)() = nullptr;
    inline void SleepThread(TimeSpan duration) {
        now_us += duration.microseconds;
        if (sleep_hook) sleep_hook();
    }
}

namespace ams { using TimeSpan = os::TimeSpan; }

namespace ams::fs {
    struct FileHandle { int mode; };
    constexpr int OpenMode_Write = 2;
    constexpr int OpenMode_AllowAppend = 4;
    enum class WriteOption { Flush };
    inline std::string contents;
    inline bool exists = false;
    inline bool fail_create = false, fail_open = false, fail_resize = false, fail_write = false;
    inline unsigned writes = 0, opens = 0, closes = 0, resizes = 0;
    inline Result CreateDirectory(const char *) { return {}; }
    inline Result DeleteFile(const char *) { exists = false; contents.clear(); return {}; }
    inline Result CreateFile(const char *, s64 size) {
        if (fail_create) return {1};
        exists = true;
        contents.resize(size);
        return {};
    }
    inline Result OpenFile(FileHandle *file, const char *, int mode) {
        if (fail_open || !exists) return {2};
        *file = {mode};
        ++opens;
        return {};
    }
    inline void CloseFile(FileHandle) { ++closes; }
    inline Result SetFileSize(FileHandle file, s64 size) {
        assert(file.mode & OpenMode_Write);
        ++resizes;
        if (fail_resize) return {3};
        contents.resize(size);
        return {};
    }
    inline Result WriteFile(FileHandle file, s64 offset, const void *data, size_t size, WriteOption) {
        assert(file.mode & OpenMode_Write);
        ++writes;
        if (fail_write) return {4};
        // Match Horizon/Atmosphere fsa::IFile::DryWrite: extension without
        // OpenMode_AllowAppend fails, unlike ordinary POSIX write().
        if (offset + size > contents.size()) {
            if (!(file.mode & OpenMode_AllowAppend)) return {5};
            contents.resize(offset + size);
        }
        std::memcpy(contents.data() + offset, data, size);
        return {};
    }
}
