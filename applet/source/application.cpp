#include <switch.h>
#include "application.hpp"

namespace mc::app {

    std::atomic<bool> exitFlag(false);

    std::unique_ptr<mc::log::Logger> log;

    const ColorSet *theme;

    uint8_t  counter = 0;

    namespace {

        const constexpr char *logLocation = "sdmc:/missioncontrol-applet.log";

        static Thread   timerThread;
        static UTimer   timer;
        
        void timerThreadFunc(void *arg) {
            Result rc;
            int idx;
            while(!mc::app::exitFlag) {
                rc = waitMulti(&idx, -1, waiterForUTimer(&timer));

                if (R_SUCCEEDED(rc)) {
                    if (idx == 0) {
                        counter = (counter + 1) % 8;
                    }
                    else {
                        break;
                    }
                }
            }
        }
    
    }

    void Initialise(void) {
        /* Initialise a logger for this applet */
        log = std::make_unique<mc::log::Logger>(logLocation);

        /* Get the current system theme */
        theme = getCurrentColorSet();

        utimerCreate(&timer, 1e8, TimerType_Repeating);
        Result rc = threadCreate(&timerThread, timerThreadFunc, NULL, NULL, 0x10000, 0x2C, -2);
        if R_FAILED(rc) 
            fatalThrow(rc);

        utimerStart(&timer);

        rc = threadStart(&timerThread);
        if (R_FAILED(rc))
            fatalThrow(rc);
        }

    void Finalise(void) {
        threadWaitForExit(&timerThread);
        threadClose(&timerThread);
    }

    void ScanInput(UserInput *input) {
        hidScanInput();
        input->kDown = hidKeysDown(CONTROLLER_P1_AUTO);
        input->kHeld = hidKeysHeld(CONTROLLER_P1_AUTO);
        hidJoystickRead(&input->leftStick, CONTROLLER_P1_AUTO, JOYSTICK_LEFT);
        hidJoystickRead(&input->rightStick, CONTROLLER_P1_AUTO, JOYSTICK_RIGHT);
        hidTouchRead(&input->touchPos, 0);
    }

}
