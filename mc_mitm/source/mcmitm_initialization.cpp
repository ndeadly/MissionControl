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
#include "mcmitm_config.hpp"
#include "bluetooth_mitm/btdrv_mitm_service.hpp"
#include "bluetooth_mitm/bluetooth/bluetooth_events.hpp"
#include "bluetooth_mitm/bluetooth/bluetooth_core.hpp"
#include "bluetooth_mitm/bluetooth/bluetooth_hid.hpp"
#include "bluetooth_mitm/bluetooth/bluetooth_ble.hpp"
 
namespace ams::mitm {

    namespace {

        constexpr size_t InitializeThreadStackSize = 0x1000;

        os::ThreadType g_initialize_thread;
        alignas(os::ThreadStackAlignment) u8 g_initialize_thread_stack[InitializeThreadStackSize];

        os::Event g_init_event(os::EventClearMode_ManualClear);

        void WaitInterfacesInitialized(void) {
            ams::bluetooth::core::WaitInitialized();
            ams::bluetooth::hid::WaitInitialized();
            if (hos::GetVersion() >= hos::Version_5_0_0)
                ams::bluetooth::ble::WaitInitialized();
        }

        void InitializeThreadFunc(void *arg) {
            // Wait for all bluetooth interfaces to be initialised
            WaitInterfacesInitialized();

            // Connect to btdrv service now that we're sure the mitm is up and running
            sm::DoWithSession([&]() {
                R_ABORT_UNLESS(btdrvInitialize());
            });

            // Start bluetooth event handling thread
            ams::bluetooth::events::Initialize();

            // Get global module settings
            auto config = GetGlobalConfig();

            // Wait for system to call BluetoothEnable so the proceeding adapter properties aren't overwritten
            ams::bluetooth::core::WaitEnabled();

            // Set bluetooth adapter host address override
            ams::bluetooth::Address null_address = {};
            if (std::memcmp(&config->bluetooth.host_address, &null_address, sizeof(ams::bluetooth::Address)) != 0) {
                R_ABORT_UNLESS(btdrvSetAdapterProperty(BtdrvBluetoothPropertyType_Address, &config->bluetooth.host_address, sizeof(ams::bluetooth::Address)));
            }

            // Set bluetooth adapter host name override
            if (std::strlen(config->bluetooth.host_name) > 0) {
                R_ABORT_UNLESS(btdrvSetAdapterProperty(BtdrvBluetoothPropertyType_Name, config->bluetooth.host_name, std::strlen(config->bluetooth.host_name)));
            }

            g_init_event.Signal();
        }

    }

    void StartInitialize(void) {
        R_ABORT_UNLESS(os::CreateThread(&g_initialize_thread, 
            InitializeThreadFunc, 
            nullptr, 
            g_initialize_thread_stack, 
            sizeof(g_initialize_thread_stack), 
            -7
        ));

        os::StartThread(&g_initialize_thread);  
    }

    void WaitInitialized(void) {
        g_init_event.Wait();
    }

}
