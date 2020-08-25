/*
 * Copyright (C) 2020 ndeadly
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <switch.h>
#include "bluetooth_events.hpp"

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
                        os::ClearSystemEvent(core::GetSystemEvent());
                        core::HandleEvent();
                        break;
                    case BtdrvEventType_BluetoothHid:
                        os::ClearSystemEvent(hid::GetSystemEvent());
                        hid::HandleEvent();
                        break;
                    case BtdrvEventType_BluetoothBle:
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
