#include <switch.h>
#include "bluetooth_events.hpp"
#include "../pscpm_module.hpp"

#include "../btdrv_mitm_logging.hpp"


namespace ams::bluetooth::events {

    namespace {

        os::ThreadType 						    g_eventHandlerThread;
        alignas(os::ThreadStackAlignment) u8 	g_eventHandlerThreadStack[0x2000];
        
        os::WaitableManagerType g_manager;
        os::WaitableHolderType 	g_holderPscPm;
        os::WaitableHolderType 	g_holderBtCore;
        os::WaitableHolderType 	g_holderBtHid;
        os::WaitableHolderType 	g_holderBtBle;

        void EventHandlerThreadFunc(void *arg) {

            // Wait for all events to be initialised
            // Todo: don't check for ble on fw < 5.0.0
            BTDRV_LOG_FMT("btdrv-mitm: Waiting for bluetooth interfaces to initialize");
            while (!(core::IsInitialized() && hid::IsInitialized() && ble::IsInitialized())) {
                svc::SleepThread(1'000'000ul);
            }

            //os::InitializeWaitableHolder(&g_holderPscPm, mitm::btdrv::GetPscPmModule()->GetEventPointer()->GetBase());
            os::InitializeWaitableHolder(&g_holderBtCore, core::GetSystemEvent());
            os::InitializeWaitableHolder(&g_holderBtHid,  hid::GetSystemEvent());
            os::InitializeWaitableHolder(&g_holderBtBle,  ble::GetSystemEvent());

            //os::SetWaitableHolderUserData(&g_holderPscPm, BtdrvEventType_PscPm);
            os::SetWaitableHolderUserData(&g_holderBtCore, BtdrvEventType_BluetoothCore);
            os::SetWaitableHolderUserData(&g_holderBtHid,  BtdrvEventType_BluetoothHid);
            os::SetWaitableHolderUserData(&g_holderBtBle,  BtdrvEventType_BluetoothBle);
            
            os::InitializeWaitableManager(&g_manager);
            //os::LinkWaitableHolder(&g_manager, &g_holderPscPm);
            os::LinkWaitableHolder(&g_manager, &g_holderBtCore);
            os::LinkWaitableHolder(&g_manager, &g_holderBtHid);
            os::LinkWaitableHolder(&g_manager, &g_holderBtBle);

            while (true) {
                BTDRV_LOG_FMT("btdrv-mitm: Waiting for events");
                auto signalled_holder = os::WaitAny(&g_manager);
                BTDRV_LOG_FMT("btdrv-mitm: Event signalled!");
                switch (os::GetWaitableHolderUserData(signalled_holder)) {
                    case BtdrvEventType_PscPm:
                        BTDRV_LOG_FMT("btdrv-mitm: handling psc:pm event");
                        mitm::btdrv::HandlePscPmEvent();
                        break;
                    case BtdrvEventType_BluetoothCore:
                        BTDRV_LOG_FMT("btdrv-mitm: handling bluetooth core event");
                        //core::GetSystemEvent()->Clear();
                        os::ClearSystemEvent(core::GetSystemEvent());
                        core::HandleEvent();
                        break;
                    case BtdrvEventType_BluetoothHid:
                        BTDRV_LOG_FMT("btdrv-mitm: handling bluetooth hid event");
                        //hid::GetSystemEvent()->Clear();
                        os::ClearSystemEvent(hid::GetSystemEvent());
                        hid::HandleEvent();
                        break;
                    case BtdrvEventType_BluetoothBle:
                        BTDRV_LOG_FMT("btdrv-mitm: handling bluetooth ble event");
                        //ble::GetSystemEvent()->Clear();
                        os::ClearSystemEvent(ble::GetSystemEvent());
                        ble::HandleEvent();
                        break;
                    default:
                        break;	
                }
            }
            BTDRV_LOG_FMT("btdrv-mitm: Wait thread exiting");
        }

    }

    Result Initialize(void) {
        BTDRV_LOG_FMT("btdrv-mitm: events Initialize");
        // Initialise power management
        //R_TRY(mitm::btdrv::InitializePscPmModule());

        R_TRY(os::CreateThread(&g_eventHandlerThread, 
            EventHandlerThreadFunc, 
            nullptr, 
            g_eventHandlerThreadStack, 
            sizeof(g_eventHandlerThreadStack), 
            9
        ));

        BTDRV_LOG_FMT("btdrv-mitm: created event handler thread");

        os::StartThread(&g_eventHandlerThread); 

        BTDRV_LOG_FMT("btdrv-mitm: started event handler thread");

        return ams::ResultSuccess();
    }

    void Finalize(void) {
        // Todo: clean up events and thread
    }

}
