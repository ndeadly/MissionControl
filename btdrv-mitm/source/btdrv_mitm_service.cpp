#include <algorithm>
#include <cstring>

#include <switch.h>
#include "btdrv_mitm_service.hpp"
#include "btdrv_shim.h"


namespace ams::mitm::btdrv {

    namespace {

        bool bluetoothInitialized   = false;
        bool hidInitialized         = false;

        os::ThreadType g_bt_event_task_thread;
	    alignas(os::ThreadStackAlignment) u8 g_bt_event_task_stack[0x1000];
        u8 g_bt_event_data_buffer[0x400];
        BluetoothEventType g_current_bt_event_type;

        os::ThreadType g_bt_hid_event_task_thread;
	    alignas(os::ThreadStackAlignment) u8 g_bt_hid_event_task_stack[0x1000];
        u8 g_bt_hid_event_data_buffer[0x480];
        HidEventType g_current_bt_hid_event_type;

        SharedMemory g_realBtShmem;
        SharedMemory g_fakeBtShmem;

        /* Actual events coming from bluetooth */
        os::SystemEventType btSystemEvent;
        os::SystemEventType btHidSystemEvent;
        os::SystemEventType btHidReportSystemEvent;

        /* Events to forward to official sysmodules */
        os::SystemEventType btSystemEventFwd;
        os::SystemEventType btHidSystemEventFwd;
        os::SystemEventType btHidReportSystemEventFwd;

        /* Secondary events the user can use to receive a copy of the real events */
        os::SystemEventType btSystemEventUser;
        os::SystemEventType btHidSystemEventUser;
        os::SystemEventType btHidReportSystemEventUser;


        void BluetoothEventThreadFunc(void *arg) {
            while (true) {
                // Wait for real bluetooth event 
                os::WaitSystemEvent(&btSystemEvent);
                
                // Signal our forwarder events
                os::SignalSystemEvent(&btSystemEventFwd);
                os::SignalSystemEvent(&btSystemEventUser);

                /*
                // Retrieve info from real event
                Result rc = btdrvGetEventInfo(&g_current_bt_event_type, g_bt_event_data_buffer, sizeof(g_bt_event_data_buffer));
                if (R_SUCCEEDED(rc)) {

                    // Perform any modifications to the data
                    BluetoothEventData *event_data = reinterpret_cast<BluetoothEventData *>(g_bt_event_data_buffer);
                    switch (g_current_bt_event_type) {
                        case BluetoothEvent_DeviceFound:
                            // Todo: try changing name and cod to look like switch pro controller
                            snprintf(event_data->deviceFound.name, sizeof(BluetoothName), "Derpy McDildoballs");
                            break;
                        case BluetoothEvent_DiscoveryStateChanged:
                        case BluetoothEvent_PinRequest:
                        case BluetoothEvent_SspRequest:
                        case BluetoothEvent_BondStateChanged:
                        default:
                            break;
                    }

                    // Signal our forwarder events
                    os::SignalSystemEvent(&btSystemEventUser);
                    os::SignalSystemEvent(&btSystemEventFwd);
                }
                */
            }
        }

        void BluetoothHidEventThreadFunc(void *arg) {
            while (true) {
                // Wait for real bluetooth event 
                os::WaitSystemEvent(&btHidSystemEvent);

                // React to real event (maybe make a copy of event buffers?)
                //btdrvGetHidEventInfo(&g_current_bt_hid_event_type, g_bt_hid_event_data_buffer, sizeof(g_bt_hid_event_data_buffer));
                    
                // Signal our forwarder events
                os::SignalSystemEvent(&btHidSystemEventFwd);
                os::SignalSystemEvent(&btHidSystemEventUser);
            }
        }

    }

    Result BtdrvMitmService::InitializeBluetooth(sf::OutCopyHandle out_handle) {

        BTDRV_LOG_FMT("btdrv-mitm: InitializeBluetooth");

        //if (this->client_info.program_id == ncm::SystemProgramId::Btm)
        if (!bluetoothInitialized) {

            Handle handle = INVALID_HANDLE;
   
            // Forward to the real bluetooth module with our event handle instead
            R_ABORT_UNLESS(btdrvInitializeBluetoothFwd(this->forward_service.get(), &handle));

            // Attach the handle to our real system event
            os::AttachReadableHandleToSystemEvent(&btSystemEvent, handle, false, os::EventClearMode_AutoClear);

            // Create forwarder events
            R_ABORT_UNLESS(os::CreateSystemEvent(&btSystemEventFwd, os::EventClearMode_AutoClear, true));
            R_ABORT_UNLESS(os::CreateSystemEvent(&btSystemEventUser, os::EventClearMode_AutoClear, true));
            
            // Set callers handle to that of our forwarder event
            //out_handle.SetValue(handle);    
            out_handle.SetValue(os::GetReadableHandleOfSystemEvent(&btSystemEventFwd)); 

            // Create and map fake bluetooth hid report shared memory
            //R_ABORT_UNLESS(shmemCreate(&g_fakeBtShmem, 0x3000, Perm_Rw, Perm_Rw));
            //R_ABORT_UNLESS(shmemMap(&g_fakeBtShmem));

            // Create thread for forwarding events
            R_ABORT_UNLESS(os::CreateThread(&g_bt_event_task_thread, 
                BluetoothEventThreadFunc, 
                nullptr, 
                g_bt_event_task_stack, 
                sizeof(g_bt_event_task_stack), 
                9
            ));

            os::StartThread(&g_bt_event_task_thread); 

            bluetoothInitialized = true;
        } else {
            //out_handle.SetValue(os::GetReadableHandleOfSystemEvent(&btSystemEvent));
            //out_handle.SetValue(os::GetReadableHandleOfSystemEvent(&btSystemEventFwd));
            out_handle.SetValue(os::GetReadableHandleOfSystemEvent(&btSystemEventUser));
        }

        return ams::ResultSuccess();
    }

    Result BtdrvMitmService::FinalizeBluetooth(void) {

        BTDRV_LOG_FMT("btdrv-mitm: FinalizeBluetooth");

        if (this->client_info.program_id == ncm::SystemProgramId::Btm) {
            R_ABORT_UNLESS(btdrvFinalizeBluetoothFwd(this->forward_service.get()));
        }

        return ams::ResultSuccess();
    }

    Result BtdrvMitmService::GetEventInfo(sf::Out<u32> out_type, const sf::OutPointerBuffer &out_buffer) {

        /*
        if (this->client_info.program_id == ncm::SystemProgramId::Btm) {
            //R_ABORT_UNLESS(btdrvGetEventInfoFwd(this->forward_service.get(), reinterpret_cast<BluetoothEventType *>(out_type.GetPointer()), out_buffer.GetPointer(), out_buffer.GetSize()));
            R_ABORT_UNLESS(btdrvGetEventInfoFwd(this->forward_service.get(), &g_current_bt_event_type, g_bt_event_data_buffer, sizeof(g_bt_event_data_buffer)));

            out_type.SetValue(g_current_bt_event_type);
            std::memcpy(out_buffer.GetPointer(), g_bt_event_data_buffer, std::min(out_buffer.GetSize(), sizeof(g_bt_event_data_buffer)));

            os::SignalSystemEvent(&btSystemEventUser);
        }
        else {
            out_type.SetValue(g_current_bt_event_type);
            std::memcpy(out_buffer.GetPointer(), g_bt_event_data_buffer, std::min(out_buffer.GetSize(), sizeof(g_bt_event_data_buffer)));
        } 
        */

        BTDRV_LOG_FMT("btdrv-mitm: GetEventInfo");

        BluetoothEventType event_type;

        R_ABORT_UNLESS(btdrvGetEventInfoFwd(this->forward_service.get(), 
            &event_type,
            static_cast<u8 *>(out_buffer.GetPointer()), 
            static_cast<size_t>(out_buffer.GetSize())
        ));

        out_type.SetValue(event_type);

        BTDRV_LOG_FMT("  event %02d", event_type);

        BluetoothEventData *event_data = reinterpret_cast<BluetoothEventData *>(out_buffer.GetPointer());
        switch (event_type) {
            case BluetoothEvent_DeviceFound:
                // Todo: try changing name and cod to look like switch pro controller
                //snprintf(event_data->deviceFound.name, sizeof(BluetoothName), "Derpy McDildoballs");
                snprintf(event_data->deviceFound.name, sizeof(BluetoothName), "Pro Controller");
                break;
            case BluetoothEvent_DiscoveryStateChanged:
            case BluetoothEvent_PinRequest:
            case BluetoothEvent_SspRequest:
            case BluetoothEvent_BondStateChanged:
            default:
                break;
        }

        /*
        if (this->client_info.program_id == ncm::SystemProgramId::Btm) {
            //os::SignalSystemEvent(&btSystemEventUser);
        }
        */   

        return ams::ResultSuccess();
    }

    Result BtdrvMitmService::InitializeHid(sf::OutCopyHandle out_handle, u16 version) {

        BTDRV_LOG_FMT("btdrv-mitm: InitializeHid");

        Handle handle = INVALID_HANDLE;

        if (!hidInitialized) {

            R_ABORT_UNLESS(btdrvInitializeHidFwd(this->forward_service.get(), &handle, version));

            // Attach the handle to our real system event
            os::AttachReadableHandleToSystemEvent(&btHidSystemEvent, handle, false, os::EventClearMode_AutoClear);

            // Create forwarder events
            R_ABORT_UNLESS(os::CreateSystemEvent(&btHidSystemEventFwd, os::EventClearMode_AutoClear, true));
            R_ABORT_UNLESS(os::CreateSystemEvent(&btHidSystemEventUser, os::EventClearMode_AutoClear, true));

            out_handle.SetValue(os::GetReadableHandleOfSystemEvent(&btHidSystemEventFwd)); 

             // Create thread for forwarding events
            R_ABORT_UNLESS(os::CreateThread(&g_bt_hid_event_task_thread, 
                BluetoothHidEventThreadFunc, 
                nullptr, 
                g_bt_hid_event_task_stack, 
                sizeof(g_bt_hid_event_task_stack), 
                9
            ));

            os::StartThread(&g_bt_hid_event_task_thread); 

            hidInitialized = true;
        }
        else {
            out_handle.SetValue(os::GetReadableHandleOfSystemEvent(&btHidSystemEventUser));
        }

        return ams::ResultSuccess();
    }

    Result BtdrvMitmService::WriteHidData(BluetoothAddress address, const sf::InPointerBuffer &buffer) {

        BTDRV_LOG_FMT("btdrv-mitm: WriteHidData (caller: %s)", this->client_info.program_id == ncm::SystemProgramId::Hid ? "HID" : "other");

        if (this->client_info.program_id == ncm::SystemProgramId::Hid) {
            /*
            Lookup controller type from address
            if (is_switch_controller) {
                 //forward command as usual
            }
            else {
                 //check outgoing packet type
                 //convert to equivalent format of target controller
            }
            */
        }

        R_ABORT_UNLESS(btdrvWriteHidDataFwd(this->forward_service.get(), 
            &address,
            reinterpret_cast<const BluetoothHidData *>(buffer.GetPointer()) 
        ));

        return ams::ResultSuccess();
    }

    Result BtdrvMitmService::FinalizeHid(void) {

        BTDRV_LOG_FMT("btdrv-mitm: FinalizeHid");

        if (this->client_info.program_id == ncm::SystemProgramId::Btm) {
            R_ABORT_UNLESS(btdrvFinalizeHidFwd(this->forward_service.get()));
        }

        return ams::ResultSuccess();
    }

    Result BtdrvMitmService::GetHidEventInfo(sf::Out<HidEventType> out_type, const sf::OutPointerBuffer &out_buffer) {

        BTDRV_LOG_FMT("btdrv-mitm: GetHidEventInfo");

        /*
        if (this->client_info.program_id == ncm::SystemProgramId::Btm) {
            // Do we need to trick btm here?
        }

        out_type.SetValue(g_current_bt_hid_event_type);
        std::memcpy(out_buffer.GetPointer(), g_bt_hid_event_data_buffer, std::min(out_buffer.GetSize(), sizeof(g_bt_hid_event_data_buffer)));
        */

        HidEventType event_type;

        R_ABORT_UNLESS(btdrvGetHidEventInfoFwd(this->forward_service.get(), 
            &event_type,
            static_cast<u8 *>(out_buffer.GetPointer()), 
            static_cast<size_t>(out_buffer.GetSize())
        ));

        out_type.SetValue(event_type);

        BTDRV_LOG_FMT("  event %02d", event_type);

        return ams::ResultSuccess();
    }

    Result BtdrvMitmService::RegisterHidReportEvent(sf::OutCopyHandle out_handle) {

        BTDRV_LOG_FMT("btdrv-mitm: RegisterHidReportEvent");

        Handle handle = INVALID_HANDLE;

        R_ABORT_UNLESS(btdrvRegisterHidReportEventFwd(this->forward_service.get(), &handle));

        out_handle.SetValue(handle);

        return ams::ResultSuccess();
    }

    // This one returns shared memory handle on 7.0.0+ 
    Result BtdrvMitmService::GetHidReportEventInfo(sf::OutCopyHandle out_handle) {

        BTDRV_LOG_FMT("btdrv-mitm: GetHidReportEventInfo");

        if (!bluetoothInitialized || hos::GetVersion() < hos::Version_7_0_0) {
            // Todo: return error
        }

        Handle handle = INVALID_HANDLE;

        R_ABORT_UNLESS(btdrvGetHidReportEventInfoFwd(this->forward_service.get(), &handle));
        
        //out_handle.SetValue(g_fakeBtShmem.handle);
        out_handle.SetValue(handle);

        return ams::ResultSuccess();
    }

    Result BtdrvMitmService::InitializeBle(sf::OutCopyHandle out_handle) {

        BTDRV_LOG_FMT("btdrv-mitm: InitializeBle");

        Handle handle = INVALID_HANDLE;

        R_ABORT_UNLESS(btdrvInitializeBleFwd(this->forward_service.get(), &handle));

        out_handle.SetValue(handle);

        return ams::ResultSuccess();
    }

    Result BtdrvMitmService::FinalizeBle(void) {

        BTDRV_LOG_FMT("btdrv-mitm: FinalizeBle");

        if (this->client_info.program_id == ncm::SystemProgramId::Btm) {
            R_ABORT_UNLESS(btdrvFinalizeBleFwd(this->forward_service.get()));
        }


        return ams::ResultSuccess();
    }

}
