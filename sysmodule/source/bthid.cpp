#include "bthid.hpp"
#include "controllermanager.hpp"

#include "logger.hpp"

namespace mc::bthid {
    
    std::atomic<bool> exitFlag(false);

    namespace {

        static Event    hidEvent = {};
        static Thread   hidEventThread;
        static uint8_t  hidEventBuffer[event_buffer_size] = {};

        static Event    hidReportEvent = {};
        static Thread   hidReportEventThread;
        static uint8_t  hidReportEventBuffer[event_buffer_size] = {};

        std::unique_ptr<mc::controller::ControllerManager> controllerManager;

        void handleConnectionStateEvent(const HidEventData *eventData) {
            //mc::log::Write("Hid Connection State: %d", eventData->connectionState.state);

            switch(eventData->connectionState.state) {
                case HidConnectionState_Connected:
                    controllerManager->attachBluetoothController(&eventData->connectionState.address);
                    break;

                case HidConnectionState_Disconnected:
                    controllerManager->removeBluetoothController(&eventData->connectionState.address);
                    break;

                default:
                    break;
            }
        }

        void hidEventThreadFunc(void* arg) {
            Result rc;
            HidEventType eventType;
            HidEventData *eventData = reinterpret_cast<HidEventData *>(hidEventBuffer);

            while (!exitFlag) {
                if R_SUCCEEDED(eventWait(&hidEvent, 1e9)) {

                    rc = btdrvHidGetEventInfo(&eventType, hidEventBuffer, sizeof(hidEventBuffer));
                    if R_FAILED(rc) 
                        fatalThrow(rc);
                    eventClear(&hidEvent);

                    switch (eventType) {
                        case HidEvent_ConnectionState:
                            handleConnectionStateEvent(eventData);
                            break;

                        default:
                            //mc::log::Write("Unexpected Hid event: %d", eventType);
                            break;
                    }
                }
            }
        }

        void hidReportEventThreadFunc(void* arg) {
            Result rc;
            HidEventType eventType;
            HidEventData *eventData = reinterpret_cast<HidEventData *>(hidReportEventBuffer);

            while (!exitFlag) {
                if R_SUCCEEDED(eventWait(&hidReportEvent, 1e9)) {

                    rc = btdrvHidGetReportEventInfo(&eventType, hidReportEventBuffer, sizeof(hidReportEventBuffer));
                    if R_FAILED(rc) 
                        fatalThrow(rc);

                    switch (eventType) {
                        case HidEvent_GetReport:
                            if (eventData->getReport.status == HidStatus_Ok) {
                                controllerManager->receiveBluetoothReport(&eventData->getReport.address, &eventData->getReport.report_data.report);
                            }
                            break;

                        default:
                            //mc::log::Write("Unexpected Hid report event: %d", eventType);
                            break;
                    }
                }
            }
        }

    }

    void Initialise(void) {
        Result rc;

        /* Init HID events */
        rc = btdrvInitializeHid(&hidEvent, 1);
        if R_FAILED(rc)
            fatalThrow(rc);
        rc = btdrvRegisterHidReportEvent(&hidReportEvent);
        if R_FAILED(rc)
            fatalThrow(rc);

        /* Init threads */
        rc = threadCreate(&hidEventThread, hidEventThreadFunc, nullptr, nullptr, thread_stack_size, 0x2C, -2);
        if R_FAILED(rc) 
            fatalThrow(rc);
        rc = threadCreate(&hidReportEventThread, hidReportEventThreadFunc, nullptr, nullptr, thread_stack_size, 0x2C, -2);
        if R_FAILED(rc) 
            fatalThrow(rc);

        /* Init controller manager */
        controllerManager = std::make_unique<mc::controller::ControllerManager>();
        controllerManager->registerBluetoothControllers();

        /* Start threads */
        rc = threadStart(&hidEventThread);
        if R_FAILED(rc) 
            fatalThrow(rc);
        rc = threadStart(&hidReportEventThread);
        if R_FAILED(rc) 
            fatalThrow(rc);
    }

    void Cleanup(void) {
        /* Wait for all threads to finish */
        threadWaitForExit(&hidReportEventThread);
        threadWaitForExit(&hidEventThread);

        /* Cleanup threads */
        threadClose(&hidReportEventThread);
        threadClose(&hidEventThread);

        /* Cleanup events */
        eventClose(&hidReportEvent);
        eventClose(&hidEvent);

        /* Cleanup Bluetooth */
        btdrvCleanupHid();
    }

    void PrepareForSleep(void) {
        controllerManager->removeControllers();
    }

}
