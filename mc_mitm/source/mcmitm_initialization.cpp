/*
 * Copyright (c) 2020-2021 ndeadly
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
#include <stratosphere.hpp>
#include <switch.h>
#include "mcmitm_initialization.hpp"
#include "bluetooth_mitm/bluetooth/bluetooth_events.hpp"
#include "bluetooth_mitm/bluetooth/bluetooth_core.hpp"
#include "bluetooth_mitm/bluetooth/bluetooth_hid.hpp"
#include "bluetooth_mitm/bluetooth/bluetooth_ble.hpp"
 
namespace ams::mitm {

    namespace {

        constexpr size_t InitializeThreadStackSize = 0x2000;

        os::ThreadType g_initialize_thread;
        alignas(os::ThreadStackAlignment) u8 g_initialize_thread_stack[InitializeThreadStackSize];

        void WaitInterfacesInitialized(void) {
            while (!(bluetooth::core::IsInitialized() && bluetooth::hid::IsInitialized() && (bluetooth::ble::IsInitialized() || (hos::GetVersion() < hos::Version_5_0_0)))) {
                svc::SleepThread(1'000'000ul);
            }
        }
        
        void InitializeThreadFunc(void *arg) {

            // Wait for all bluetooth interfaces to be initialised
            WaitInterfacesInitialized();

            // Connect to btdrv service now that we're up and running
            sm::DoWithSession([&]() {
                R_ABORT_UNLESS(btdrvInitialize());
            });

            // Start bluetooth event handling thread
            bluetooth::events::Initialize();

            // Parse config ini

            // Set host name override

            // Set host address override


        }

    }

    void StartInitialize(void) {

        R_ABORT_UNLESS(os::CreateThread(&g_initialize_thread, 
            InitializeThreadFunc, 
            nullptr, 
            g_initialize_thread_stack, 
            sizeof(g_initialize_thread_stack), 
            9
        ));

        os::StartThread(&g_initialize_thread);  
    }

}
