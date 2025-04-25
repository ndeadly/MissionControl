/*
 * Copyright (c) 2020-2025 ndeadly
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
#include "bluetooth_hid_report.hpp"
#include "bluetooth_circular_buffer.hpp"
#include "../btdrv_shim.h"
#include "../btdrv_mitm_flags.hpp"
#include "../../controllers/controller_management.hpp"

namespace ams::bluetooth::hid::report {

    namespace {

        constexpr size_t BluetoothSharedMemorySize = 0x3000;

        constexpr s32 ThreadPriority = -11;
        constexpr size_t ThreadStackSize = 0x1000;
        alignas(os::ThreadStackAlignment) constinit u8 g_thread_stack[ThreadStackSize];
        constinit os::ThreadType g_thread;

        // This is only required  on fw < 7.0.0
        constinit bluetooth::HidReportEventInfo g_event_info;
        constinit bluetooth::HidEventType g_current_event_type;

        os::SystemEvent g_system_event;
        os::SystemEvent g_system_event_fwd(os::EventClearMode_AutoClear, true);
        os::SystemEvent g_system_event_user_fwd(os::EventClearMode_AutoClear, true);

        os::Event g_init_event(os::EventClearMode_ManualClear);
        os::Event g_report_read_event(os::EventClearMode_AutoClear);

        bool g_should_forward_report = true;

        os::SharedMemory g_real_bt_shmem;
        os::SharedMemory g_fake_bt_shmem(BluetoothSharedMemorySize, os::MemoryPermission_ReadWrite, os::MemoryPermission_ReadWrite);

        bluetooth::CircularBuffer *g_real_buffer;
        bluetooth::CircularBuffer *g_fake_buffer;

        constinit bluetooth::HidReportEventInfo g_fake_report_event_info;

        void EventThreadFunc(void *) {

            WaitInitialized();
            for (;;) {
                g_system_event.Wait();
                HandleEvent();
            }
        }

    }

    bool IsInitialized() {
        return g_init_event.TryWait();
    }

    void WaitInitialized() {
        g_init_event.Wait();
    }

    void SignalInitialized() {
        g_init_event.Signal();
    }

    void ForwardHidReportEvent() {
        g_should_forward_report = true;
        g_report_read_event.Signal();
    }

    void ConsumeHidReportEvent() {
        g_should_forward_report = false;
        g_report_read_event.Signal();
    }

    os::SharedMemory *GetRealSharedMemory() {
        if (hos::GetVersion() < hos::Version_7_0_0) {
            return nullptr;
        }

        return &g_real_bt_shmem;
    }

    os::SharedMemory *GetFakeSharedMemory() {
        return &g_fake_bt_shmem;
    }

    os::SystemEvent *GetSystemEvent() {
        return &g_system_event;
    }

    os::SystemEvent *GetForwardEvent() {
        return &g_system_event_fwd;
    }

    os::SystemEvent *GetUserForwardEvent() {
        return &g_system_event_user_fwd;
    }

    Result Initialize() {
        R_TRY(os::CreateThread(&g_thread,
            EventThreadFunc,
            nullptr,
            g_thread_stack,
            ThreadStackSize,
            ThreadPriority
        ));

        os::SetThreadNamePointer(&g_thread, "mc::HidReportThread");
        os::StartThread(&g_thread);

        R_SUCCEED();
    }

    void Finalize() {
        os::DestroyThread(&g_thread);
    }

    Result MapRemoteSharedMemory(os::NativeHandle handle) {
        g_real_bt_shmem.Attach(BluetoothSharedMemorySize, handle, true);
        g_real_bt_shmem.Map(os::MemoryPermission_ReadWrite);
        g_real_buffer = reinterpret_cast<bluetooth::CircularBuffer *>(g_real_bt_shmem.GetMappedAddress());

        R_SUCCEED();
    }

    Result InitializeReportBuffer() {
        g_fake_bt_shmem.Map(os::MemoryPermission_ReadWrite);

        auto event_info = reinterpret_cast<bluetooth::BufferedEventInfo *>(g_fake_bt_shmem.GetMappedAddress());
        event_info->buffer.Initialize("HID Report");
        event_info->type = bluetooth::EventBufferType_HidReport;
        event_info->ready = true;

        g_fake_buffer = &event_info->buffer;

        R_SUCCEED();
    }

    Result WriteHidDataReport(const bluetooth::Address address, const bluetooth::HidReport *report) {
        if (hos::GetVersion() >= hos::Version_9_0_0) {
            g_fake_report_event_info.data_report.v9.addr = address;
            std::memcpy(&g_fake_report_event_info.data_report.v9.report, report, report->size + sizeof(report->size));
        } else {
            // Todo: check this may still be necessary
            //g_fake_report_event_info.data_report.v7.size = g_fake_report_event_info.data_report.v7.report.size + 0x11;
            g_fake_report_event_info.data_report.v7.addr = address;
            std::memcpy(&g_fake_report_event_info.data_report.v7.report, report, report->size + sizeof(report->size));
        }

        g_fake_buffer->Write(hos::GetVersion() >= hos::Version_12_0_0 ? BtdrvHidEventType_Data : BtdrvHidEventTypeOld_Data, &g_fake_report_event_info, report->size + 0x11);
        g_system_event_fwd.Signal();

        R_SUCCEED();
    }

    Result WriteHidSetReport(const bluetooth::Address address, u32 status) {
        g_fake_report_event_info.set_report.addr = address;
        g_fake_report_event_info.set_report.res = status;

        g_fake_buffer->Write(hos::GetVersion() >= hos::Version_12_0_0 ? BtdrvHidEventType_Data : BtdrvHidEventTypeOld_Data, &g_fake_report_event_info, sizeof(g_fake_report_event_info.set_report));
        g_system_event_fwd.Signal();

        R_SUCCEED();
    }

    Result WriteHidGetReport(const bluetooth::Address address, const bluetooth::HidReport *report) {
        if (hos::GetVersion() >= hos::Version_9_0_0) {
            g_fake_report_event_info.get_report.v9.addr = address;
            g_fake_report_event_info.get_report.v9.res = 0;
            std::memcpy(&g_fake_report_event_info.get_report.v9.report, report, report->size + sizeof(report->size));
        } else {
            g_fake_report_event_info.get_report.v1.addr = address;
            g_fake_report_event_info.get_report.v1.res = 0;
            std::memcpy(&g_fake_report_event_info.get_report.v1.report, report, report->size + sizeof(report->size));
        }

        g_fake_buffer->Write(hos::GetVersion() >= hos::Version_12_0_0 ? BtdrvHidEventType_GetReport : BtdrvHidEventTypeOld_GetReport, &g_fake_report_event_info, report->size + 0x11);
        g_system_event_fwd.Signal();

        R_SUCCEED();
    }

    /* Only used for < 7.0.0. Newer firmwares read straight from shared memory */
    Result GetEventInfo(bluetooth::HidEventType *type, void *buffer, size_t size) {
        AMS_UNUSED(size);

        while (true) {
            auto packet = g_fake_buffer->Read();
            if (!packet) {
                return -1;
            }

            g_fake_buffer->Free();

            auto event_info = reinterpret_cast<bluetooth::HidReportEventInfo *>(buffer);
            *type = static_cast<bluetooth::HidEventType>(packet->header.type);

            switch (packet->header.type) {
                case 0xff:
                    continue;
                case BtdrvHidEventTypeOld_Data:
                    event_info->data_report.v1.hdr.addr = packet->data.data_report.v7.addr;
                    event_info->data_report.v1.hdr.res = 0;
                    event_info->data_report.v1.hdr.size = packet->header.size;
                    event_info->data_report.v1.addr = packet->data.data_report.v7.addr;
                    std::memcpy(&event_info->data_report.v1.report, &packet->data.data_report.v7.report, packet->header.size);
                    break;
                case BtdrvHidEventTypeOld_SetReport:
                    event_info->set_report.addr = packet->data.set_report.addr;
                    event_info->set_report.res = packet->data.set_report.res;
                    break;
                case BtdrvHidEventTypeOld_GetReport:
                    event_info->get_report.v1.addr = packet->data.get_report.v1.addr;
                    event_info->get_report.v1.res = packet->data.get_report.v1.res;
                    std::memcpy(&event_info->get_report.v1.report, &packet->data.get_report.v1.report, packet->header.size);
                    break;
                default:
                    break;
            }
        }

        R_SUCCEED();
    }

    inline void HandleHidReportEventV1() {
        R_ABORT_UNLESS(btdrvGetHidReportEventInfo(&g_event_info, sizeof(bluetooth::HidReportEventInfo), &g_current_event_type));

        switch (g_current_event_type) {
            case BtdrvHidEventTypeOld_Data:
                {
                    auto device = controller::LocateHandler(&g_event_info.data_report.v1.addr);
                    if (device) {
                        device->HandleDataReportEvent(&g_event_info);
                    }
                }
                break;
            case BtdrvHidEventTypeOld_SetReport:
                {
                    auto device = controller::LocateHandler(&g_event_info.set_report.addr);
                    if (device) {
                        device->HandleSetReportEvent(&g_event_info);
                    }
                }
                break;
            case BtdrvHidEventTypeOld_GetReport:
                {
                    auto device = controller::LocateHandler(&g_event_info.get_report.v1.addr);
                    if (device) {
                        device->HandleGetReportEvent(&g_event_info);
                    }
                }
                break;
            default:
                break;
        }
    }

    inline void HandleHidReportEventV7() {
        while (true) {
            auto real_packet = g_real_buffer->Read();
            if (!real_packet) {
                break;
            }

            g_real_buffer->Free();

            switch (real_packet->header.type) {
                case 0xff:
                    continue;
                case BtdrvHidEventTypeOld_Data:
                    {
                        auto device = controller::LocateHandler(hos::GetVersion() < hos::Version_9_0_0 ? &real_packet->data.data_report.v7.addr : &real_packet->data.data_report.v9.addr);
                        if (device) {
                            device->HandleDataReportEvent(&real_packet->data);
                        }
                    }
                    break;
                case BtdrvHidEventTypeOld_SetReport:
                    {
                        auto device = controller::LocateHandler(&real_packet->data.set_report.addr);
                        if (device) {
                            device->HandleSetReportEvent(&real_packet->data);
                        }
                    }
                    break;
                case BtdrvHidEventTypeOld_GetReport:
                    {
                        auto device = controller::LocateHandler(&real_packet->data.get_report.v1.addr);
                        if (device) {
                            device->HandleGetReportEvent(&real_packet->data);
                        }
                    }
                    break;
                default:
                    break;
            }
        }
    }

    inline void HandleHidReportEventV12() {
        while (true) {
            auto real_packet = g_real_buffer->Read();
            if (!real_packet) {
                break;
            }

            g_real_buffer->Free();

            switch (real_packet->header.type) {
                case 0xff:
                    continue;
                case BtdrvHidEventType_Data:
                    {
                        auto device = controller::LocateHandler(&real_packet->data.data_report.v9.addr);
                        if (device) {
                            device->HandleDataReportEvent(&real_packet->data);
                        }
                    }
                    break;
                case BtdrvHidEventType_SetReport:
                    {
                        auto device = controller::LocateHandler(&real_packet->data.set_report.addr);
                        if (device) {
                            device->HandleSetReportEvent(&real_packet->data);
                        }
                    }
                    break;
                case BtdrvHidEventType_GetReport:
                    {
                        auto device = controller::LocateHandler(&real_packet->data.get_report.v9.addr);
                        if (device) {
                            device->HandleGetReportEvent(&real_packet->data);
                        }
                    }
                    break;
                default:
                    break;
            }
        }
    }

    void HandleEvent() {
        if (g_redirect_hid_report_events) {
            g_system_event_user_fwd.Signal();
            g_report_read_event.Wait();

            // Return early if event was consumed by the client
            if (!g_should_forward_report) {
                return;
            }
        }

        if (hos::GetVersion() >= hos::Version_12_0_0) {
            HandleHidReportEventV12();
        } else if (hos::GetVersion() >= hos::Version_7_0_0) {
            HandleHidReportEventV7();
        } else {
            HandleHidReportEventV1();
        }
    }

}
