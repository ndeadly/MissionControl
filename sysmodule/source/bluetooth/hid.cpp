#include <cstring>
#include "bluetooth/hid.hpp"
#include "gamepad/controllermanager.hpp"

#include "log.hpp"

namespace mc::bluetooth::hid {
    
    std::atomic<bool> exitFlag(false);

    namespace {

        static Event    btmDeviceInfoEvent = {};
        static Event    btmDeviceConditionEvent = {};
        static Thread   btmDeviceEventThread;
        static BtmDeviceCondition connectedDevices = {};

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
                if (R_SUCCEEDED(eventWait(&hidEvent, 1e9))) {

                    rc = btdrvGetHidEventInfo(&eventType, hidEventBuffer, sizeof(hidEventBuffer));
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
                if (R_SUCCEEDED(eventWait(&hidReportEvent, 1e9))) {

                    rc = btdrvGetHidReportEventInfo(&eventType, hidReportEventBuffer, sizeof(hidReportEventBuffer));
                    if R_FAILED(rc) 
                        fatalThrow(rc);

                    switch (eventType) {
                        case HidEvent_GetReport:
                            if (eventData->getReport.status == HidStatus_Ok) {
                                //mc::log::WriteData(&eventData->getReport.report_data.report, eventData->getReport.report_data.size);
                                controllerManager->receiveBluetoothReport(&eventData->getReport.address, &eventData->getReport.report_data.report);
                            }
                            break;

                        default:
                            mc::log::Write("Unexpected Hid report event: %d", eventType);
                            mc::log::WriteData(&eventData->getReport.report_data.report, eventData->getReport.report_data.size);
                            break;
                    }
                }
            }
        }

    }
    
    void btmDeviceEventThreadFunc(void* arg) {
        Result rc;
        BtmDeviceCondition devices;

        while (!exitFlag) {
            if (R_SUCCEEDED(eventWait(&btmDeviceConditionEvent, 1e9))) {

                rc = btmGetDeviceCondition(&devices);
                if R_FAILED(rc)
                    fatalThrow(rc);
                eventClear(&btmDeviceConditionEvent);

                mc::log::Write("Device condition event");
                for (int i = 0; i < devices.connected_count; ++i) {
                    mc::log::WriteData(&devices.devices[i], sizeof(BtmConnectedDevice));
                }


                int i;
                if (devices.connected_count > connectedDevices.connected_count) {
                    i = devices.connected_count - 1;
                    controllerManager->attachBluetoothController(&devices.devices[i].address);
                }
                else if (devices.connected_count < connectedDevices.connected_count) {
                    /*
                    for (i = 0; i < ) {

                    }
                    controllerManager->removeBluetoothController();
                    */
                }

                std::memcpy(&connectedDevices, &devices, sizeof(BtmDeviceCondition));

            }
        }
    }

    void Initialise(void) {
        Result rc;
        
        /* Init HID events */
        rc = btdrvInitializeHid(&hidEvent, 1);
        if R_FAILED(rc)
            fatalThrow(rc);

        uint8_t flags;
        rc = btmAcquireDeviceConditionEvent(&btmDeviceConditionEvent, &flags);
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

        rc = threadCreate(&btmDeviceEventThread, btmDeviceEventThreadFunc, nullptr, nullptr, thread_stack_size, 0x2C, -2);
        if R_FAILED(rc) 
            fatalThrow(rc);

        /* Init controller manager */
        controllerManager = std::make_unique<mc::controller::ControllerManager>();
        //controllerManager->registerBluetoothControllers();

        /* Start threads */
        rc = threadStart(&hidEventThread);
        if R_FAILED(rc) 
            fatalThrow(rc);

        rc = threadStart(&hidReportEventThread);
        if R_FAILED(rc) 
            fatalThrow(rc);

        /*
        rc = threadStart(&btmDeviceEventThread);
        if R_FAILED(rc) 
            fatalThrow(rc);
        */
    }

    void Cleanup(void) {
        /* Wait for all threads to finish */
        threadWaitForExit(&btmDeviceEventThread);
        threadWaitForExit(&hidReportEventThread);
        threadWaitForExit(&hidEventThread);

        /* Cleanup threads */
        threadClose(&btmDeviceEventThread);
        threadClose(&hidReportEventThread);
        threadClose(&hidEventThread);

        /* Cleanup events */
        eventClose(&btmDeviceConditionEvent);
        eventClose(&hidReportEvent);
        eventClose(&hidEvent);

        /* Cleanup Bluetooth HID interface */
        //btdrvFinalizeHid();
    }

    void PrepareForSleep(void) {
        mc::log::Write("Console preparing for sleep");
        controllerManager->removeControllers();
    }

    void PrepareForWake(void) {
        //controllerManager->registerBluetoothControllers();
        mc::log::Write("Console preparing for wake");
    }

    void OnWake(void) {
        mc::log::Write("Console awake");
    }

}
