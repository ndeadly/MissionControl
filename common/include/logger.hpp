#pragma once

namespace mc::log {

    //const constexpr char *app = "sdmc:/missioncontrol-applet.log";
    //const constexpr char *sys = "sdmc:/missioncontrol-sysmodule.log";

    //void Write(const char *location, const char *fmt, ...);
    //void WriteData(const char *location, void *data, size_t size);

    class Logger {

        public:
            Logger(const char *location);

            void write(const char *fmt, ...);
            void writeData(void *data, size_t size);

        private:
            Mutex m_logMutex;
            const char *m_location;

    };

}
