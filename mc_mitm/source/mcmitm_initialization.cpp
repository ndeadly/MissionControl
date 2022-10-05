/*
 * Copyright (c) 2020-2022 ndeadly
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
#include "async/async.hpp"
#include "bluetooth_mitm/btdrv_mitm_service.hpp"
#include "bluetooth_mitm/bluetoothmitm_module.hpp"
#include "btm_mitm/btmmitm_module.hpp"
#include "mc/mc_module.hpp"
#include "bluetooth_mitm/bluetooth/bluetooth_events.hpp"
#include "bluetooth_mitm/bluetooth/bluetooth_core.hpp"
#include "bluetooth_mitm/bluetooth/bluetooth_hid.hpp"
#include "bluetooth_mitm/bluetooth/bluetooth_hid_report.hpp"
#include "bluetooth_mitm/bluetooth/bluetooth_ble.hpp"

namespace ams::mitm {

    namespace {

        const s32 ThreadPriority = -7;
        const size_t ThreadStackSize = 0x1000;
        alignas(os::ThreadStackAlignment) u8 g_thread_stack[ThreadStackSize];
        os::ThreadType g_thread;

        os::Event g_init_event(os::EventClearMode_ManualClear);

        void InitializeThreadFunc(void *) {
            // Start async worker thread(s)
            ams::async::Initialize();

            // Start bluetooth event handling thread
            ams::bluetooth::events::Initialize();

            // Start hid report handling thread
            ams::bluetooth::hid::report::Initialize();

            // Wait for system to call BluetoothEnable
            ams::bluetooth::core::WaitEnabled();

            // Connect to btdrv service now that we're sure the mitm is up and running
            R_ABORT_UNLESS(btdrvInitialize());

            // Get global module settings
            auto config = GetGlobalConfig();

            // Set bluetooth adapter host address override
            ams::bluetooth::Address null_address = {};
            if (std::memcmp(&config->bluetooth.host_address, &null_address, sizeof(ams::bluetooth::Address)) != 0) {
                if (hos::GetVersion() < hos::Version_12_0_0) {
                    R_ABORT_UNLESS(btdrvLegacySetAdapterProperty(BtdrvBluetoothPropertyType_Address, &config->bluetooth.host_address, sizeof(ams::bluetooth::Address)));
                }
                else {
                    BtdrvAdapterProperty property;
                    property.type = BtdrvAdapterPropertyType_Address;
                    property.size = sizeof(ams::bluetooth::Address);
                    std::memcpy(property.data, &config->bluetooth.host_address, sizeof(ams::bluetooth::Address));
                    R_ABORT_UNLESS(btdrvSetAdapterProperty(BtdrvAdapterPropertyType_Address, &property));
                }
            }

            // Set bluetooth adapter host name override
            if (std::strlen(config->bluetooth.host_name) > 0) {
                if (hos::GetVersion() < hos::Version_12_0_0) {
                    R_ABORT_UNLESS(btdrvLegacySetAdapterProperty(BtdrvBluetoothPropertyType_Name, config->bluetooth.host_name, std::strlen(config->bluetooth.host_name)));
                }
                else {
                    BtdrvAdapterProperty property;
                    property.type = BtdrvAdapterPropertyType_Name;
                    property.size = std::strlen(config->bluetooth.host_name);
                    std::memcpy(property.data, config->bluetooth.host_name, std::strlen(config->bluetooth.host_name));
                    R_ABORT_UNLESS(btdrvSetAdapterProperty(BtdrvAdapterPropertyType_Name, &property));
                }
            }

            g_init_event.Signal();
        }

    }

    void StartInitialize() {
        R_ABORT_UNLESS(os::CreateThread(&g_thread,
            InitializeThreadFunc,
            nullptr,
            g_thread_stack,
            ThreadStackSize,
            ThreadPriority
        ));

        os::SetThreadNamePointer(&g_thread, "mc::InitThread");
        os::StartThread(&g_thread);
    }

    void WaitInitialized() {
        g_init_event.Wait();
    }

    void LaunchModules() {
        R_ABORT_UNLESS(ams::mitm::bluetooth::Launch());
        R_ABORT_UNLESS(ams::mitm::btm::Launch());
        R_ABORT_UNLESS(ams::mitm::mc::Launch());
    }

    void WaitModules() {
        ams::mitm::mc::WaitFinished();
        ams::mitm::btm::WaitFinished();
        ams::mitm::bluetooth::WaitFinished();
    }

}
