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
#include "bluetooth_hid_report.hpp"
#include "bluetooth_circular_buffer.hpp"
#include "../btdrv_shim.h"
#include "../btdrv_mitm_flags.hpp"
#include "../../mcmitm_utils.hpp"
#include "../../controllers/controller_management.hpp"
#include <mutex>
#include <cstring>

namespace ams::bluetooth::hid::report {

    namespace {

        constexpr auto bluetooth_sharedmem_size = 0x3000;

        os::ThreadType g_event_handler_thread;
        alignas(os::ThreadStackAlignment) uint8_t g_event_handler_thread_stack[0x1000];
        s32 g_event_handler_thread_priority = mitm::utils::ConvertToUserPriority(17);

        // This is only required  on fw < 7.0.0
        bluetooth::HidEventInfo g_event_info;
        bluetooth::HidEventType g_current_event_type;

        os::SystemEvent g_system_event;
        os::SystemEvent g_system_event_fwd(os::EventClearMode_AutoClear, true);
        os::SystemEvent g_system_event_user_fwd(os::EventClearMode_AutoClear, true);

        os::Event g_init_event(os::EventClearMode_ManualClear);

        SharedMemory g_real_bt_shmem;
        SharedMemory g_fake_bt_shmem;

        bluetooth::CircularBuffer *g_real_buffer;
        bluetooth::CircularBuffer *g_fake_buffer;

        bluetooth::HidReportData g_fake_report_data;

        Service *g_forward_service;
        os::ThreadId g_main_thread_id;

        void EventThreadFunc(void *arg) {
            while (true) {
                g_system_event.Wait();
                HandleEvent();
            }
        }

    }

    bool IsInitialized() {
        return g_init_event.TryWait();
    }

    void WaitInitialized(void) {
        g_init_event.Wait();
    }

    SharedMemory *GetRealSharedMemory(void) {
        if (hos::GetVersion() < hos::Version_7_0_0)
            return nullptr;

        return &g_real_bt_shmem;
    }

    SharedMemory *GetFakeSharedMemory(void) {
        return &g_fake_bt_shmem;
    }

    os::SystemEvent *GetSystemEvent(void) {
        return &g_system_event;
    }

    os::SystemEvent *GetForwardEvent(void) {
        return &g_system_event_fwd;
    }

    os::SystemEvent *GetUserForwardEvent(void) {
        return &g_system_event_user_fwd;
    }

    Result Initialize(Handle event_handle, Service *forward_service, os::ThreadId main_thread_id) {
        g_system_event.AttachReadableHandle(event_handle, false, os::EventClearMode_AutoClear);

        R_TRY(os::CreateThread(&g_event_handler_thread, 
            EventThreadFunc, 
            nullptr, 
            g_event_handler_thread_stack, 
            sizeof(g_event_handler_thread_stack), 
            g_event_handler_thread_priority
        ));

        g_forward_service = forward_service;
        g_main_thread_id = main_thread_id;

        os::StartThread(&g_event_handler_thread); 

        g_init_event.Signal();

        return ams::ResultSuccess();
    }

    void Finalize(void) {
        os::DestroyThread(&g_event_handler_thread);
    }

    Result MapRemoteSharedMemory(Handle handle) {
        shmemLoadRemote(&g_real_bt_shmem, handle, bluetooth_sharedmem_size, Perm_Rw);
        R_TRY(shmemMap(&g_real_bt_shmem));
        g_real_buffer = reinterpret_cast<bluetooth::CircularBuffer *>(shmemGetAddr(&g_real_bt_shmem));

        return ams::ResultSuccess();
    }

    Result InitializeReportBuffer(void) {
        R_TRY(shmemCreate(&g_fake_bt_shmem, bluetooth_sharedmem_size, Perm_Rw, Perm_Rw));
        R_TRY(shmemMap(&g_fake_bt_shmem));
        g_fake_buffer = reinterpret_cast<bluetooth::CircularBuffer *>(shmemGetAddr(&g_fake_bt_shmem));

        g_fake_buffer->Initialize("HID Report");
        g_fake_buffer->type = bluetooth::CircularBufferType_HidReport;
        g_fake_buffer->_unk3 = 1;

        return ams::ResultSuccess();
    }

    Result WriteHidReportBuffer(const bluetooth::Address *address, const bluetooth::HidReport *report) {
        if (hos::GetVersion() < hos::Version_9_0_0) {
            g_fake_report_data.v1.size = g_fake_report_data.report.size + 0x11;
            std::memcpy(&g_fake_report_data.v1.address, address, sizeof(bluetooth::Address));
        }
        else {
            std::memcpy(&g_fake_report_data.v9.address, address, sizeof(bluetooth::Address));
        }
        std::memcpy(&g_fake_report_data.report, report, report->size + sizeof(report->size));

        g_fake_buffer->Write(BtdrvHidEventType_GetReport, &g_fake_report_data, g_fake_report_data.report.size + 0x11); 

        g_system_event_fwd.Signal();

        return ams::ResultSuccess();
    }

    Result SendHidReport(const bluetooth::Address *address, const bluetooth::HidReport *report) {
        if (os::GetThreadId(os::GetCurrentThread()) == g_main_thread_id)
            R_TRY(btdrvWriteHidDataFwd(g_forward_service, address, report));
        else
            R_TRY(btdrvWriteHidData(*address, report));

        return ams::ResultSuccess();
    }

    /* Only used for < 7.0.0. Newer firmwares read straight from shared memory */ 
    Result GetEventInfo(bluetooth::HidEventType *type, uint8_t* buffer, size_t size) {
        while (true) {
            auto packet = g_fake_buffer->Read();
            if (!packet)
                return -1;

            g_fake_buffer->Free();

            if (packet->header.type == 0xff) {
                continue;
            }
            else {
                auto event_info = reinterpret_cast<bluetooth::HidEventInfo *>(buffer);

                *type = static_cast<bluetooth::HidEventType>(packet->header.type);
                std::memcpy(&event_info->get_report.address, &packet->data.v1.address, sizeof(bluetooth::Address));
                event_info->get_report.status = BtdrvHidStatus_Ok;
                event_info->get_report.report_length = packet->header.size;

                std::memcpy(&event_info->get_report.report_data, &packet->data, packet->header.size);
                break;
            }      
        }
        
        return ams::ResultSuccess();
    }

    void HandleEvent(void) {
        if (!g_redirect_hid_report_events) {
            if (hos::GetVersion() < hos::Version_7_0_0) {
                R_ABORT_UNLESS(btdrvGetHidReportEventInfo(&g_event_info, sizeof(bluetooth::HidEventInfo), &g_current_event_type));

                switch (g_current_event_type) {
                    case BtdrvHidEventType_GetReport:
                        {
                            auto device = controller::LocateHandler(&g_event_info.get_report.address);
                            if (!device)
                                return;

                            device->HandleIncomingReport(&g_event_info.get_report.report_data.report);
                        }
                        break;
                    default:
                        g_fake_buffer->Write(g_current_event_type, &g_event_info.get_report.report_data, g_event_info.get_report.report_length);
                        break;
                }
            }
            else {
                while (true) {
                    auto real_packet = g_real_buffer->Read();
                    if (!real_packet)
                        break;

                    g_real_buffer->Free();

                    switch (real_packet->header.type) {
                        case 0xff:
                            continue;
                        case BtdrvHidEventType_GetReport:
                            {
                                auto device = controller::LocateHandler(hos::GetVersion() < hos::Version_9_0_0 ? &real_packet->data.v1.address : &real_packet->data.v9.address);
                                if (!device)
                                    continue;

                                device->HandleIncomingReport(&real_packet->data.report);
                            }
                            break;
                        default:
                            g_fake_buffer->Write(real_packet->header.type, &real_packet->data, real_packet->header.size);
                            break;
                    }
                } 
            }
        }
        else {
            g_system_event_user_fwd.Signal();
        }
    }

}
