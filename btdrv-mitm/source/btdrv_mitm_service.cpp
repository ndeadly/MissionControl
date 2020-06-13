#include <algorithm>
#include <atomic>
#include <memory>
#include <vector>
#include <cstring>

#include <switch.h>
#include "btdrv_mitm_service.hpp"
#include "btdrv_mitm_flags.hpp"
#include "btdrv_shim.h"

#include "bluetooth/bluetooth_events.hpp"
#include "controllermanager.hpp"


namespace ams::mitm::btdrv {

    namespace {

        bool g_bluetoothInitialized = false;
        bool g_hidInitialized       = false;
        bool g_hidReportInitialized = false;
        bool g_bleInitialized       = false;
       

        /*
        void pscpmThreadFunc(void *arg) {
            psc::PmModule   pmModule;
            psc::PmState    pmState;
            psc::PmFlagSet  pmFlags;

            // Init power management
            psc::PmModuleId pmModuleId = static_cast<psc::PmModuleId>(0xbd);
            const psc::PmModuleId dependencies[] = { psc::PmModuleId_Bluetooth }; //PscPmModuleId_Bluetooth, PscPmModuleId_Btm, PscPmModuleId_Hid ??
            R_ABORT_UNLESS(pmModule.Initialize(pmModuleId, dependencies, util::size(dependencies), os::EventClearMode_AutoClear));

            while (true) {
                // Check power management events
                pmModule.GetEventPointer()->Wait();

                if (R_SUCCEEDED(pmModule.GetRequest(&pmState, &pmFlags))) {
                    switch(pmState) {
                        case PscPmState_Awake:
                            break;
                        case PscPmState_ReadyAwaken:
                            g_preparingForSleep = false;
                            BTDRV_LOG_FMT("Console waking up");
                            break;
                        case PscPmState_ReadySleep:
                            g_preparingForSleep = true;
                            BTDRV_LOG_FMT("Console going to sleep");
                            break;
                        case PscPmState_ReadyShutdown:
                        case PscPmState_ReadyAwakenCritical:              
                        case PscPmState_ReadySleepCritical:
                        default:
                            break;
                    }

                    R_ABORT_UNLESS(pmModule.Acknowledge(pmState, ams::ResultSuccess()));
                }
                
            }

            pmModule.Finalize();
        }
        */


    }

    Result BtdrvMitmService::InitializeBluetooth(sf::OutCopyHandle out_handle) {

        BTDRV_LOG_FMT("btdrv-mitm: InitializeBluetooth");

        //if (this->client_info.program_id == ncm::SystemProgramId::Btm)
        if (!g_bluetoothInitialized) {

            Handle handle = INVALID_HANDLE;
   
            // Forward to the real bluetooth module with our event handle instead
            R_ABORT_UNLESS(btdrvInitializeBluetoothFwd(this->forward_service.get(), &handle));

            // Attach the handle to our real system event
            os::AttachReadableHandleToSystemEvent(&g_btSystemEvent, handle, false, os::EventClearMode_AutoClear);

            // Create forwarder events
            R_ABORT_UNLESS(InitializeBluetoothCoreEvents());
            
            // Set callers handle to that of our forwarder event
            out_handle.SetValue(os::GetReadableHandleOfSystemEvent(&g_btSystemEventFwd)); 

            // Create and map fake bluetooth hid report shared memory
            R_ABORT_UNLESS(InitializeBluetoothHidReportFakeSharedMemory());

            // Create thread for forwarding events
            R_ABORT_UNLESS(StartBluetoothCoreEventThread());

            g_bluetoothInitialized = true;

        } else {
            out_handle.SetValue(os::GetReadableHandleOfSystemEvent(&g_btSystemEventUser));
        }

        return ams::ResultSuccess();
    }

    Result BtdrvMitmService::FinalizeBluetooth(void) {

        BTDRV_LOG_FMT("btdrv-mitm: FinalizeBluetooth");

        // Only btm should be able to make this call
        if (this->client_info.program_id == ncm::SystemProgramId::Btm) {
            R_ABORT_UNLESS(btdrvFinalizeBluetoothFwd(this->forward_service.get()));
        }

        return ams::ResultSuccess();
    }

    /*
    Result BtdrvMitmService::CancelBond(BluetoothAddress address) {

        BTDRV_LOG_FMT("btdrv-mitm: CancelBond");

        R_ABORT_UNLESS(btdrvCancelBondFwd(this->forward_service.get(), &address));

        return ams::ResultSuccess();
    }
    */


    Result BtdrvMitmService::GetEventInfo(sf::Out<u32> out_type, const sf::OutPointerBuffer &out_buffer) {

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

        size_t data_size;
        switch (event_type) {
            case BluetoothEvent_DeviceFound:
                data_size = sizeof(event_data->deviceFound);
                // Todo: try changing name and cod to look like switch pro controller
                //snprintf(event_data->deviceFound.name, sizeof(BluetoothName), "Pro Controller");
                //event_data->deviceFound._unk2 = 0xffffffcb;
                break;
            case BluetoothEvent_DiscoveryStateChanged:
                data_size = sizeof(event_data->discoveryState);
                break;
            case BluetoothEvent_PinRequest:
                data_size = sizeof(event_data->pinReply);
                break;
            case BluetoothEvent_SspRequest:
                data_size = sizeof(event_data->sspReply);
                break;
            case BluetoothEvent_BondStateChanged:
                data_size = sizeof(event_data->bondState.v2);
                break;
            default:
                data_size = out_buffer.GetSize();
                break;
        }

        BTDRV_LOG_DATA(out_buffer.GetPointer(), data_size);

        return ams::ResultSuccess();
    }

    Result BtdrvMitmService::InitializeHid(sf::OutCopyHandle out_handle, u16 version) {

        BTDRV_LOG_FMT("btdrv-mitm: InitializeHid");

        Handle handle = INVALID_HANDLE;

        if (!g_hidInitialized) {

            R_ABORT_UNLESS(btdrvInitializeHidFwd(this->forward_service.get(), &handle, version));

            // Attach the handle to our real system event
            os::AttachReadableHandleToSystemEvent(&g_btHidSystemEvent, handle, false, os::EventClearMode_AutoClear);

            // Create forwarder events
            R_ABORT_UNLESS(InitializeBluetoothHidEvents());

            out_handle.SetValue(os::GetReadableHandleOfSystemEvent(&g_btHidSystemEventFwd)); 

            // Create thread for forwarding events
            R_ABORT_UNLESS(StartBluetoothHidEventThread());

            g_hidInitialized = true;
        }
        else {
            out_handle.SetValue(os::GetReadableHandleOfSystemEvent(&g_btHidSystemEventUser));
        }

        return ams::ResultSuccess();
    }

    Result BtdrvMitmService::WriteHidData(BluetoothAddress address, const sf::InPointerBuffer &buffer) {

        //BTDRV_LOG_FMT("btdrv-mitm: WriteHidData (caller: %s)", this->client_info.program_id == ncm::SystemProgramId::Hid ? "HID" : "other");

        if (this->client_info.program_id == ncm::SystemProgramId::Hid) {
            auto controller = locateController(&address);

            if (!controller->isSwitchController()) {
                 BTDRV_LOG_FMT("btdrv-mitm: WriteHidData - Non-Switch controller");
            }
        }

        R_ABORT_UNLESS(btdrvWriteHidDataFwd(this->forward_service.get(), 
            &address,
            reinterpret_cast<const BluetoothHidData *>(buffer.GetPointer()) 
        ));

        return ams::ResultSuccess();
    }

    Result BtdrvMitmService::FinalizeHid(void) {

        BTDRV_LOG_FMT("btdrv-mitm: FinalizeHid");

        // Only btm should be able to make this call
        if (this->client_info.program_id == ncm::SystemProgramId::Btm) {
            R_ABORT_UNLESS(btdrvFinalizeHidFwd(this->forward_service.get()));
        }

        return ams::ResultSuccess();
    }

    Result BtdrvMitmService::GetHidEventInfo(sf::Out<u32> out_type, const sf::OutPointerBuffer &out_buffer) {

        BTDRV_LOG_FMT("btdrv-mitm: GetHidEventInfo");

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

    /* 1.0.0 - 3.0.2 */
    Result BtdrvMitmService::RegisterHidReportEventDeprecated(sf::OutCopyHandle out_handle) {
        return RegisterHidReportEvent(out_handle);
    }

    /* 4.0.0+ */
    Result BtdrvMitmService::RegisterHidReportEvent(sf::OutCopyHandle out_handle) {

        BTDRV_LOG_FMT("btdrv-mitm: RegisterHidReportEvent");

        if (!g_hidReportInitialized) {
            Handle handle = INVALID_HANDLE;

            R_ABORT_UNLESS(btdrvRegisterHidReportEventFwd(this->forward_service.get(), &handle));

            // Attach the handle to our real system event
            os::AttachReadableHandleToSystemEvent(&g_btHidReportSystemEvent, handle, false, os::EventClearMode_AutoClear);

             // Create forwarder events
            R_ABORT_UNLESS(InitializeBluetoothHidReportEvents());

            // Set callers handle to that of our forwarder event
            out_handle.SetValue(os::GetReadableHandleOfSystemEvent(&g_btHidReportSystemEventFwd));

            // Create thread for forwarding events
            R_ABORT_UNLESS(StartBluetoothHidReportEventThread());

            g_hidReportInitialized = true;
        }
        else {
            out_handle.SetValue(os::GetReadableHandleOfSystemEvent(&g_btHidReportSystemEventUser));
        }

        return ams::ResultSuccess();
    }

    /* 1.0.0 - 3.0.2 */
    Result BtdrvMitmService::GetHidReportEventInfoDeprecated1(sf::Out<u32> out_type, const sf::OutPointerBuffer &out_buffer) {
        
        BTDRV_LOG_FMT("btdrv-mitm: GetHidReportEventInfo");

        HidEventType event_type;

        R_ABORT_UNLESS(btdrvGetHidReportEventInfoDeprecatedFwd(this->forward_service.get(), 
            &event_type,
            static_cast<u8 *>(out_buffer.GetPointer()), 
            static_cast<size_t>(out_buffer.GetSize())
        ));

        out_type.SetValue(event_type);

        return ams::ResultSuccess();
    }

    /* 4.0.0 - 6.2.0 */
    Result BtdrvMitmService::GetHidReportEventInfoDeprecated2(sf::Out<u32> out_type, const sf::OutPointerBuffer &out_buffer) {

        BTDRV_LOG_FMT("btdrv-mitm: GetHidReportEventInfo");

        HidEventType event_type;

        R_ABORT_UNLESS(btdrvGetHidReportEventInfoDeprecatedFwd(this->forward_service.get(), 
            &event_type,
            static_cast<u8 *>(out_buffer.GetPointer()), 
            static_cast<size_t>(out_buffer.GetSize())
        ));

        out_type.SetValue(event_type);

        return ams::ResultSuccess();
    }

    /* 7.0.0+ */
    Result BtdrvMitmService::GetHidReportEventInfo(sf::OutCopyHandle out_handle) {

        BTDRV_LOG_FMT("btdrv-mitm: GetHidReportEventInfo");

        Handle handle = INVALID_HANDLE;

        R_ABORT_UNLESS(btdrvGetHidReportEventInfoFwd(this->forward_service.get(), &handle));

        // Load and map the real bluetooth shared memory
        shmemLoadRemote(&g_realBtShmem, handle, BLUETOOTH_SHAREDMEM_SIZE, Perm_Rw);
        R_ABORT_UNLESS(shmemMap(&g_realBtShmem));
        g_realCircBuff = reinterpret_cast<bluetooth::CircularBuffer *>(shmemGetAddr(&g_realBtShmem));
        BTDRV_LOG_FMT("Real shmem @ 0x%p", (void *)g_realCircBuff);
       
        // Return the handle of our fake shared memory to the caller instead
        out_handle.SetValue(g_fakeBtShmem.handle);
        
        return ams::ResultSuccess();
    }

    Result BtdrvMitmService::InitializeBle(sf::OutCopyHandle out_handle) {

        BTDRV_LOG_FMT("btdrv-mitm: InitializeBle");

        Handle handle = INVALID_HANDLE;

        if (!g_bleInitialized) {

            R_ABORT_UNLESS(btdrvInitializeBleFwd(this->forward_service.get(), &handle));

            // Attach the handle to our real system event
            os::AttachReadableHandleToSystemEvent(&g_btBleSystemEvent, handle, false, os::EventClearMode_AutoClear);

            // Create forwarder events
            R_ABORT_UNLESS(InitializeBluetoothBleEvents());

            out_handle.SetValue(os::GetReadableHandleOfSystemEvent(&g_btBleSystemEventFwd)); 

            // Create thread for forwarding events
            R_ABORT_UNLESS(StartBluetoothBleEventThread());

            g_bleInitialized = true;
        }
        else {
            out_handle.SetValue(os::GetReadableHandleOfSystemEvent(&g_btBleSystemEventUser));
        }

        return ams::ResultSuccess();
    }

    Result BtdrvMitmService::FinalizeBle(void) {

        BTDRV_LOG_FMT("btdrv-mitm: FinalizeBle");

        if (this->client_info.program_id == ncm::SystemProgramId::Btm) {
            R_ABORT_UNLESS(btdrvFinalizeBleFwd(this->forward_service.get()));
        }

        return ams::ResultSuccess();
    }

    Result BtdrvMitmService::RedirectSystemEvents(bool redirect) {

        BTDRV_LOG_FMT("btdrv-mitm: RedirectSystemEvents");

        g_redirectEvents = redirect;

        return ams::ResultSuccess();
    }

}
