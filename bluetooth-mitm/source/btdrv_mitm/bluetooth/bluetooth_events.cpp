/*
 * Copyright (c) 2020 ndeadly
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <switch.h>
#include "bluetooth_events.hpp"

namespace ams::bluetooth::events {

    namespace {

        os::ThreadType 						        g_event_handler_thread;
        alignas(os::ThreadStackAlignment) uint8_t   g_event_handler_thread_stack[0x2000];
        
        os::WaitableManagerType g_manager;
        os::WaitableHolderType 	g_holder_bt_core;
        os::WaitableHolderType 	g_holder_bt_hid;
        os::WaitableHolderType 	g_holder_bt_ble;

        void EventHandlerThreadFunc(void *arg) {

            // Wait for all events to be initialised
            while (!(core::IsInitialized() && hid::IsInitialized() && (ble::IsInitialized() || (hos::GetVersion() < hos::Version_5_0_0)))) {
                svc::SleepThread(1'000'000ul);
            }

            // Initialise the btdrv service now that we can be sure the MITM is up and running
            sm::DoWithSession([&]() {
                R_ABORT_UNLESS(btdrvInitialize());
            });

            os::InitializeWaitableManager(&g_manager);

            os::InitializeWaitableHolder(&g_holder_bt_core, core::GetSystemEvent());
            os::SetWaitableHolderUserData(&g_holder_bt_core, BtdrvEventType_BluetoothCore);
            os::LinkWaitableHolder(&g_manager, &g_holder_bt_core);

            os::InitializeWaitableHolder(&g_holder_bt_hid, hid::GetSystemEvent());
            os::SetWaitableHolderUserData(&g_holder_bt_hid, BtdrvEventType_BluetoothHid);
            os::LinkWaitableHolder(&g_manager, &g_holder_bt_hid);

            if (hos::GetVersion() >= hos::Version_5_0_0) {
                os::InitializeWaitableHolder(&g_holder_bt_ble, ble::GetSystemEvent());
                os::SetWaitableHolderUserData(&g_holder_bt_ble, BtdrvEventType_BluetoothBle);
                os::LinkWaitableHolder(&g_manager, &g_holder_bt_ble);
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

        R_TRY(os::CreateThread(&g_event_handler_thread, 
            EventHandlerThreadFunc, 
            nullptr, 
            g_event_handler_thread_stack, 
            sizeof(g_event_handler_thread_stack), 
            9
        ));

        os::StartThread(&g_event_handler_thread); 

        return ams::ResultSuccess();
    }

    void Finalize(void) {
        os::DestroyThread(&g_event_handler_thread);
    }

}
