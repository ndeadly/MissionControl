#pragma once

#include <atomic>
#include <memory>
#include <switch.h>

#include "gfx/theme.h"
#include "logger.hpp"

namespace mc::app {

    extern std::atomic<bool> exitFlag;

    extern std::unique_ptr<mc::log::Logger> log;

    extern const ColorSet *theme;

    extern uint8_t  counter;

    struct UserInput {
        uint64_t kDown;
        uint64_t kHeld;
        JoystickPosition leftStick;
        JoystickPosition rightStick;
        touchPosition touchPos;
    };

    void Initialise(void);
    void Finalise(void);

    void ScanInput(UserInput *input);

}
