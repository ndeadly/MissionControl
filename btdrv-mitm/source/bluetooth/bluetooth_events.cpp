#include <switch.h>
#include "bluetooth_events.hpp"
#include "../pscpm_module.hpp"

namespace ams::bluetooth::events {

    namespace {

        os::ThreadType 						    g_eventHandlerThread;
        alignas(os::ThreadStackAlignment) u8 	g_eventHandlerThreadStack[0x2000];
        
        os::WaitableManagerType g_manager;
        os::WaitableHolderType 	g_holderPscPm;
        os::WaitableHolderType 	g_holderBtCore;
        os::WaitableHolderType 	g_holderBtHid;
        os::WaitableHolderType 	g_holderBtBle;

    }

    void EventHandlerThreadFunc(void *arg) {
        while (true) {
            auto signalled_holder = os::WaitAny(&g_manager);
            switch (os::GetWaitableHolderUserData(signalled_holder)) {
                case BtdrvEventType_PscPm:
                    mitm::btdrv::handlePscPmEvent();
                    break;
                case BtdrvEventType_BluetoothCore:
                    core::HandleEvent();
                    break;
                case BtdrvEventType_BluetoothHid:
                    hid::HandleEvent();
                    break;
                case BtdrvEventType_BluetoothBle:
                    ble::HandleEvent();
                    break;
                default:
                    break;	
            }
        }
    }

    void AttachWaitHolder(BtdrvEventType type) {
        switch(type) {
            case BtdrvEventType_PscPm:
                os::InitializeWaitableHolder(&g_holderPscPm, mitm::btdrv::GetPscPmModule()->GetEventPointer()->GetBase());
                os::SetWaitableHolderUserData(&g_holderPscPm, 	BtdrvEventType_PscPm);
                os::LinkWaitableHolder(&g_manager, &g_holderPscPm);
                break;
            case BtdrvEventType_BluetoothCore:
                os::InitializeWaitableHolder(&g_holderBtCore, core::GetSystemEvent());
                os::SetWaitableHolderUserData(&g_holderBtCore, 	BtdrvEventType_BluetoothCore);
                os::LinkWaitableHolder(&g_manager, &g_holderBtCore);
                break;
            case BtdrvEventType_BluetoothHid:
                os::InitializeWaitableHolder(&g_holderBtHid, hid::GetSystemEvent());
                os::SetWaitableHolderUserData(&g_holderBtHid, 	BtdrvEventType_BluetoothHid);
                os::LinkWaitableHolder(&g_manager, &g_holderBtHid);
                break;
            case BtdrvEventType_BluetoothBle:
                os::InitializeWaitableHolder(&g_holderBtBle, ble::GetSystemEvent());
                os::SetWaitableHolderUserData(&g_holderBtBle, 	BtdrvEventType_BluetoothBle);
                os::LinkWaitableHolder(&g_manager, &g_holderBtBle);
                break;
            default:
                return;
        }
    }

    Result InitializeSystemEvents(void) {
        //R_TRY(InitializePscPmModule());
        //R_TRY(core::InitializeEvents());
        //R_TRY(hid::InitializeEvents());
        //R_TRY(ble::InitializeEvents());
        
        os::InitializeWaitableManager(&g_manager);

        return ams::ResultSuccess();
    }
    
    Result StartEventHandlerThread(void) {
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

}
