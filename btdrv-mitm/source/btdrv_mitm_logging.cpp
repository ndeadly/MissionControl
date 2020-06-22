#include "btdrv_mitm_logging.hpp"

#include <stratosphere.hpp>
#include <mutex>
#include <cstdio>
#include <ctype.h>

namespace ams::mitm::btdrv {

    static ams::os::Mutex g_log_lock(true);
    static bool has_cleared_logs = false;
    static constexpr const char LogFile[] = "sdmc:/btdrv-mitm.log";

    inline static void ClearLogs() {
        std::scoped_lock lk(g_log_lock);
        remove(LogFile);
    }

    void LogBase(const char *fmt, std::va_list args) {
        std::scoped_lock lk(g_log_lock);
        if(!has_cleared_logs) {
            ClearLogs();
            has_cleared_logs = true;
        }
        auto file = fopen(LogFile, "a+");
        if(file) {
            vfprintf(file, fmt, args);
            fclose(file);
        }
    }

    void LogData(void *data, size_t size) {
        std::scoped_lock lk(g_log_lock);

        auto file = std::fopen(LogFile, "a+");

        unsigned int i = 0;
        while (i < size) {
            // Print offset
            std::fprintf(file, " %04x", i);
            std::fprintf(file, " |");

            // Print line of hex
            unsigned int j;
            for (j = 0; j < 16; ++j) {
                if (i + j < size) {
                    std::fprintf(file, " %02x", ((char *)data)[i+j]);
                }
                else {
                    break;
                }
            }

            // Print separator
            for (unsigned int k = 0; k < 16-j; ++k) 
            {
                fprintf(file, "   ");
            }
            fprintf(file, " | ");

            // Print line of ascii
            for (unsigned int j = 0; j < 16; ++j) {
                if (i + j < size) {
                    char c = ((char *)data)[i+j];
                    std::fprintf(file, "%c", isprint(c) ? c: 0x2e);
                }
                else {
                    break;
                }
            }

            std::fprintf(file, "\n");
            i += 16;
        }
        std::fprintf(file, "\n");
        std::fclose(file);

    }

    void LogDataMsg(void *data, size_t size, const char *fmt, ...) {
        std::va_list args;
        va_start(args, fmt);

        std::scoped_lock lk(g_log_lock);
        auto file = std::fopen(LogFile, "a+");
        if(file) {
            std::vfprintf(file, fmt, args);
            
            unsigned int i = 0;
            while (i < size) {
                // Print offset
                std::fprintf(file, " %04x", i);
                std::fprintf(file, " |");

                // Print line of hex
                unsigned int j;
                for (j = 0; j < 16; ++j) {
                    if (i + j < size) {
                        std::fprintf(file, " %02x", ((char *)data)[i+j]);
                    }
                    else {
                        break;
                    }
                }

                // Print separator
                for (unsigned int k = 0; k < 16-j; ++k) 
                {
                    fprintf(file, "   ");
                }
                fprintf(file, " | ");

                // Print line of ascii
                for (unsigned int j = 0; j < 16; ++j) {
                    if (i + j < size) {
                        char c = ((char *)data)[i+j];
                        std::fprintf(file, "%c", isprint(c) ? c: 0x2e);
                    }
                    else {
                        break;
                    }
                }

                std::fprintf(file, "\n");
                i += 16;
            }
            std::fprintf(file, "\n");
            std::fclose(file);
        }

        va_end(args);
    }

}