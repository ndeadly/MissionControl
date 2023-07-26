/*
 * Copyright (c) 2020-2023 ndeadly
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
#include "mc_usb_handler.hpp"
#include "../controllers/dualshock3_controller.hpp"

namespace ams::usb {

    namespace {

        Result GetOldestPairedDeviceAddress(bluetooth::Address *out_address) {
            if (hos::GetVersion() >= hos::Version_13_0_0) {
                s32 total_out;
                BtmDeviceInfoV13 device_info[10];
                R_TRY(btmGetDeviceInfo(BtmProfile_Hid, device_info, 10, &total_out));

                *out_address = device_info[0].addr;
            } else {
                BtmDeviceInfoList device_info_list;
                R_TRY(btmLegacyGetDeviceInfo(&device_info_list));

                *out_address = device_info_list.devices[0].addr;
            }

            R_SUCCEED();
        }

        Result HandleUsbHsInterfaceAvailableEvent() {
            s32 total_entries = 0;
            UsbHsInterface interfaces[8] = {};
            R_TRY(usbHsQueryAvailableInterfaces(controller::Dualshock3Controller::GetUsbInterfaceFilter(), interfaces, sizeof(interfaces), &total_entries));

            for(int i = 0; i < total_entries; ++i) {
                if (controller::Dualshock3Controller::UsbIdentify(&interfaces[i])) {
                    bool pairing_started = false;
                    R_TRY(btmsysIsGamepadPairingStarted(&pairing_started));
                    if (pairing_started) {
                        // Make room for a new device if pairing database is full
                        u8 paired_count;
                        R_TRY(btmsysGetPairedGamepadCount(&paired_count));
                        if (paired_count >= 10) {
                            // Get the address of the least recently connected device in the pairing database
                            bluetooth::Address address;
                            R_TRY(GetOldestPairedDeviceAddress(&address));

                            // Remove the bonded address to make room for our new pairing
                            R_TRY(btmRemoveDeviceInfo(address));
                            R_TRY(btdrvRemoveBond(address));
                        }

                        // Pair the Dualshock 3 controller via USB
                        R_TRY(controller::Dualshock3Controller::UsbPair(&interfaces[i]));
                    }
                }
            }

            R_SUCCEED();
        }

        const s32 ThreadPriority = 9;
        const size_t ThreadStackSize = 0x4000;
        alignas(os::ThreadStackAlignment) u8 g_thread_stack[ThreadStackSize];
        os::ThreadType g_thread;

        void UsbThreadFunction(void *) {
            Event if_event;
            R_ABORT_UNLESS(usbHsCreateInterfaceAvailableEvent(&if_event, true, 0, controller::Dualshock3Controller::GetUsbInterfaceFilter()));

            os::SystemEvent interface_available_event;
            interface_available_event.AttachReadableHandle(if_event.revent, false, os::EventClearMode_AutoClear);

            for (;;) {
                interface_available_event.Wait();
                R_ABORT_UNLESS(HandleUsbHsInterfaceAvailableEvent());
            }

            usbHsDestroyInterfaceAvailableEvent(&if_event, 0);
        }

    }

    void Launch() {
        R_ABORT_UNLESS(os::CreateThread(&g_thread,
            UsbThreadFunction,
            nullptr,
            g_thread_stack,
            ThreadStackSize,
            ThreadPriority
        ));

        os::SetThreadNamePointer(&g_thread, "mc::UsbThread");
        os::StartThread(&g_thread);
    }

    void WaitFinished() {
        os::WaitThread(&g_thread);
    }

}
