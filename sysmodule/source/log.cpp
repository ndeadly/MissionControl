#include "log.hpp"

#include <ctype.h>
#include <switch.h>

namespace mc::log {

    namespace {

        Mutex logMutex;
        const constexpr char *location = "sdmc:/missioncontrol-sysmodule.log";

    }

    void Write(const char *fmt, ...) {
        mutexLock(&logMutex);

        FILE *fp = std::fopen(location, "a");

        va_list va;
        va_start(va, fmt);
        std::vfprintf(fp, fmt, va);
        va_end(va);

        std::fprintf(fp, "\n");
        std::fclose(fp);

        mutexUnlock(&logMutex);
    }

    void WriteData(void *data, size_t size) {
        mutexLock(&logMutex);

        FILE *fp = std::fopen(location, "a");

        unsigned int i = 0;
        while (i < size) {
            // Print offset
            std::fprintf(fp, " %04x", i);
            std::fprintf(fp, " |");

            // Print line of hex
            unsigned int j;
            for (j = 0; j < 16; ++j) {
                if (i + j < size) {
                    std::fprintf(fp, " %02x", ((char *)data)[i+j]);
                }
                else {
                    break;
                }
            }

            // Print separator
            for (unsigned int k = 0; k < 16-j; ++k) 
            {
                fprintf(fp, "   ");
            }
            fprintf(fp, " | ");

            // Print line of ascii
            for (unsigned int j = 0; j < 16; ++j) {
                if (i + j < size) {
                    char c = ((char *)data)[i+j];
                    std::fprintf(fp, "%c", isprint(c) ? c: 0x2e);
                }
                else {
                    break;
                }
            }

            std::fprintf(fp, "\n");
            i += 16;
        }
        std::fclose(fp);

        mutexUnlock(&logMutex);
    }
    
}
