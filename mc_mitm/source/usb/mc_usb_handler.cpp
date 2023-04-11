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

        const UsbHsInterfaceFilter g_if_filter = {
            .Flags = UsbHsInterfaceFilterFlags_bcdDevice_Min | UsbHsInterfaceFilterFlags_bInterfaceClass,
            .bcdDevice_Min = 0,
            .bInterfaceClass = USB_CLASS_HID,
        };

        Result HandleUsbHsInterfaceAvailableEvent() {
            s32 total_entries = 0;
            UsbHsInterface interfaces[8] = {};
            R_TRY(usbHsQueryAvailableInterfaces(&g_if_filter, interfaces, sizeof(interfaces), &total_entries));

            for(int i = 0; i < total_entries; ++i) {
                if (controller::Dualshock3Controller::UsbIdentify(&interfaces[i])) {
                    bool pairing_started = false;
                    R_TRY(btmsysIsGamepadPairingStarted(&pairing_started));
                    if (pairing_started) {
                        R_TRY(controller::Dualshock3Controller::UsbPair(&interfaces[i]));
                    }
                }
            }

            R_SUCCEED();
        }

        const s32 ThreadPriority = 9;
        const size_t ThreadStackSize = 0x2000;
        alignas(os::ThreadStackAlignment) u8 g_thread_stack[ThreadStackSize];
        os::ThreadType g_thread;

        void UsbThreadFunction(void *) {
            Event if_event;
            R_ABORT_UNLESS(usbHsCreateInterfaceAvailableEvent(&if_event, true, 0, &g_if_filter));

            os::SystemEvent interface_available_event;
            interface_available_event.AttachReadableHandle(if_event.revent, false, os::EventClearMode_AutoClear);

            for (;;) {
                interface_available_event.Wait();
                R_ABORT_UNLESS(HandleUsbHsInterfaceAvailableEvent());
            }

            usbHsDestroyInterfaceAvailableEvent(&if_event, 0);
        }

    }

    Result Launch() {
        R_TRY(os::CreateThread(&g_thread,
            UsbThreadFunction,
            nullptr,
            g_thread_stack,
            ThreadStackSize,
            ThreadPriority
        ));

        os::SetThreadNamePointer(&g_thread, "mc::UsbThread");
        os::StartThread(&g_thread);

        return ams::ResultSuccess();
    }

    void WaitFinished() {
        os::WaitThread(&g_thread);
    }

}
