#include <switch.h>
#include "bluetooth_events.hpp"
#include "../btdrv_mitm_logging.hpp"


namespace ams::bluetooth::events {

    namespace {

        os::ThreadType 						    g_eventHandlerThread;
        alignas(os::ThreadStackAlignment) u8 	g_eventHandlerThreadStack[0x2000];
        
        os::WaitableManagerType g_manager;
        os::WaitableHolderType 	g_holderBtCore;
        os::WaitableHolderType 	g_holderBtHid;
        os::WaitableHolderType 	g_holderBtBle;

        void EventHandlerThreadFunc(void *arg) {

            // Wait for all events to be initialised
            while (!(core::IsInitialized() && hid::IsInitialized() && (ble::IsInitialized() || (hos::GetVersion() < hos::Version_5_0_0)))) {
                svc::SleepThread(1'000'000ul);
            }

            os::InitializeWaitableManager(&g_manager);

            os::InitializeWaitableHolder(&g_holderBtCore, core::GetSystemEvent());
            os::SetWaitableHolderUserData(&g_holderBtCore, BtdrvEventType_BluetoothCore);
            os::LinkWaitableHolder(&g_manager, &g_holderBtCore);

            os::InitializeWaitableHolder(&g_holderBtHid,  hid::GetSystemEvent());
            os::SetWaitableHolderUserData(&g_holderBtHid,  BtdrvEventType_BluetoothHid);
            os::LinkWaitableHolder(&g_manager, &g_holderBtHid);

            if (hos::GetVersion() >= hos::Version_5_0_0) {
                os::InitializeWaitableHolder(&g_holderBtBle,  ble::GetSystemEvent());
                os::SetWaitableHolderUserData(&g_holderBtBle,  BtdrvEventType_BluetoothBle);
                os::LinkWaitableHolder(&g_manager, &g_holderBtBle);
            }

            while (true) {
                auto signalled_holder = os::WaitAny(&g_manager);
                switch (os::GetWaitableHolderUserData(signalled_holder)) {
                    case BtdrvEventType_BluetoothCore:
                        //core::GetSystemEvent()->Clear();
                        os::ClearSystemEvent(core::GetSystemEvent());
                        core::HandleEvent();
                        break;
                    case BtdrvEventType_BluetoothHid:
                        //hid::GetSystemEvent()->Clear();
                        os::ClearSystemEvent(hid::GetSystemEvent());
                        hid::HandleEvent();
                        break;
                    case BtdrvEventType_BluetoothBle:
                        //ble::GetSystemEvent()->Clear();
                        os::ClearSystemEvent(ble::GetSystemEvent());
                        ble::HandleEvent();
                        break;
                    default:
                        break;	
                }
            }
        }

    }

    Result Initialize(void) {
        BTDRV_LOG_FMT("btdrv-mitm: events Initialize");

        R_TRY(os::CreateThread(&g_eventHandlerThread, 
            EventHandlerThreadFunc, 
            nullptr, 
            g_eventHandlerThreadStack, 
            sizeof(g_eventHandlerThreadStack), 
            9
        ));

        os::StartThread(&g_eventHandlerThread); 

        return ams::ResultSuccess();
    }

    void Finalize(void) {
        os::DestroyThread(&g_eventHandlerThread);
    }

}
