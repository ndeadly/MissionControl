#include <algorithm>
#include <atomic>
#include <memory>
#include <vector>
#include <cstring>

#include <switch.h>
#include "btdrv_mitm_service.hpp"
#include "btdrv_shim.h"

#include "circularbuffer.hpp"
#include "controllermanager.hpp"


#include "controllers/bluetoothcontroller.hpp"
#include "controllers/switchcontroller.hpp"


namespace ams::mitm::btdrv {

    namespace {

        bool g_preparingForSleep    = false;
        bool g_redirectEvents       = false;

        bool g_bluetoothInitialized = false;
        bool g_hidInitialized       = false;
        bool g_hidReportInitialized = false;
        bool g_bleInitialized       = false;

        os::ThreadType g_bt_event_task_thread;
	    alignas(os::ThreadStackAlignment) u8 g_bt_event_task_stack[0x2000];
        //u8 g_bt_event_data_buffer[0x400];
        //BluetoothEventType g_current_bt_event_type;

        os::ThreadType g_bt_hid_event_task_thread;
	    alignas(os::ThreadStackAlignment) u8 g_bt_hid_event_task_stack[0x2000];
        u8 g_bt_hid_event_data_buffer[0x480];
        HidEventType g_current_bt_hid_event_type;

        os::ThreadType g_bt_hid_report_event_task_thread;
	    alignas(os::ThreadStackAlignment) u8 g_bt_hid_report_event_task_stack[0x2000];

        os::ThreadType g_bt_ble_event_task_thread;
	    alignas(os::ThreadStackAlignment) u8 g_bt_ble_event_task_stack[0x1000];
        //u8 g_bt_ble_event_data_buffer[0x400];
        //BluetoothEventType g_current_bt_ble_event_type;

        SharedMemory g_realBtShmem;
        SharedMemory g_fakeBtShmem;

        bluetooth::CircularBuffer *g_realCircBuff;
        bluetooth::CircularBuffer *g_fakeCircBuff;

        /* Actual events coming from bluetooth */
        os::SystemEventType g_btSystemEvent;
        os::SystemEventType g_btHidSystemEvent;
        os::SystemEventType g_btHidReportSystemEvent;
        os::SystemEventType g_btBleSystemEvent;

        /* Events to forward to official sysmodules */
        os::SystemEventType g_btSystemEventFwd;
        os::SystemEventType g_btHidSystemEventFwd;
        os::SystemEventType g_btHidReportSystemEventFwd;
        os::SystemEventType g_btBleSystemEventFwd;

        /* Secondary events the user can use to receive a copy of the real events */
        os::SystemEventType g_btSystemEventUser;
        os::SystemEventType g_btHidSystemEventUser;
        os::SystemEventType g_btHidReportSystemEventUser;
        os::SystemEventType g_btBleSystemEventUser;


        u8 g_fakeReportBuffer[0x42] = {};
        HidReportData *g_fakeReportData = reinterpret_cast<HidReportData *>(g_fakeReportBuffer);

        Result ProcessHidReportPackets(bluetooth::CircularBuffer *realBuffer, bluetooth::CircularBuffer *fakeBuffer) {
            controller::BluetoothController *controller;
            bluetooth::CircularBufferPacket *realPacket;          

            /*
            BTDRV_LOG_FMT("realBuffer: name: %s, roffs: %d, woffs: %d, capacity: %d, _unk3: %d, id: %d", 
                realBuffer->name,
                realBuffer->readOffset, 
                realBuffer->writeOffset,
                realBuffer->size,
                realBuffer->_unk3,
                realBuffer->id);
            BTDRV_LOG_FMT("fakeBuffer: name: %s, roffs: %d, woffs: %d, capacity: %d, _unk3: %d, id: %d", 
                fakeBuffer->name,
                fakeBuffer->readOffset, 
                fakeBuffer->writeOffset, 
                fakeBuffer->size,
                fakeBuffer->_unk3,
                fakeBuffer->id);
            */
            

            // Take snapshot of current write offset
            u32 writeOffset = realBuffer->writeOffset;

            while (true) {
                if (realBuffer->readOffset == writeOffset)
                    break;

                // Get packet from real buffer
                //realPacket = reinterpret_cast<bluetooth::CircularBufferPacket *>(realBuffer->_read());
                realPacket = reinterpret_cast<bluetooth::CircularBufferPacket *>(&realBuffer->data[realBuffer->readOffset]);
                if (!realPacket)
                    break;

                // Move read pointer past current packet (I think this is what Free does)
                if (realBuffer->readOffset != writeOffset) {
                    u32 newOffset = realBuffer->readOffset + realPacket->header.size + sizeof(bluetooth::CircularBufferPacketHeader);
                    if (newOffset >= BLUETOOTH_CIRCBUFFER_SIZE)
                        newOffset = 0;

                    realBuffer->_setReadOffset(newOffset);
                }
                
                //BTDRV_LOG_DATA(&realPacket->data, realPacket->header.size);
                //BTDRV_LOG_DATA(realPacket, realPacket->header.size + sizeof(bluetooth::CircularBufferPacketHeader));
                //BTDRV_LOG_FMT("fakeBuffer: [%d] writing %d bytes to data[%d]", realPacket->header.type, realPacket->header.size + sizeof(bluetooth::CircularBufferPacketHeader), fakeBuffer->writeOffset);

                switch (realPacket->header.type) {
                    case 0xff:
                        // Skip over packet type 0xff. This packet indicates the buffer should wrap around on next read.
                        // Since our buffer read and write offsets can differ from the real buffer we want to write this ourselves 
                        // when appropriate via CircularBuffer::Write()
                        continue;
                        
                    case 4:
                        {
                            // Locate the controller that sent the report
                            controller = locateController(hos::GetVersion() < hos::Version_9_0_0 ? &realPacket->data.address : &realPacket->data.v2.address);
                            if (!controller) {
                                continue;
                            } 
                            
                            if (controller->isSwitchController()) {
                                // Write unmodified packet directly to fake buffer (_write call will add new timestamp)
                                fakeBuffer->Write(realPacket->header.type, &realPacket->data, realPacket->header.size);
                            }
                            else {
                                const HidReport *inReport;
                                HidReport *outReport;
                                // copy address and stuff over
                                if (ams::hos::GetVersion() < ams::hos::Version_10_0_0) {
                                    g_fakeReportData->size = 0;    // Todo: calculate size of report data
                                    std::memcpy(&g_fakeReportData->address, &realPacket->data.address, sizeof(BluetoothAddress));
                                    inReport = &realPacket->data.report;
                                    outReport = &g_fakeReportData->report;
                                }
                                else {
                                    std::memcpy(&g_fakeReportData->v2.address, &realPacket->data.v2.address, sizeof(BluetoothAddress));
                                    inReport = &realPacket->data.v2.report;
                                    outReport = &g_fakeReportData->v2.report;
                                }

                                auto switchData = reinterpret_cast<controller::SwitchReportData *>(&outReport->data);
                                switchData->report0x30.timer = os::ConvertToTimeSpan(realPacket->header.timestamp).GetMilliSeconds() & 0xff;

                                // Translate packet to switch pro format
                                controller->convertReportFormat(inReport, outReport);
                                //BTDRV_LOG_DATA(g_fakeReportData, sizeof(g_fakeReportBuffer));

                                // Write the converted report to our fake buffer
                                fakeBuffer->Write(4, g_fakeReportData, sizeof(g_fakeReportBuffer));                
                            }
                        }
                        break;

                    default:
                        BTDRV_LOG_FMT("unknown packet received: %d", realPacket->header.type);
                        fakeBuffer->Write(realPacket->header.type, &realPacket->data, realPacket->header.size);
                        break;
                }

            } 

            return ams::ResultSuccess();
        }

        void pscpmThreadFunc(void *arg) {
            psc::PmModule   pmModule;
            psc::PmState    pmState;
            psc::PmFlagSet  pmFlags;

            /* Init power management */
            psc::PmModuleId pmModuleId = static_cast<psc::PmModuleId>(0xbd);
            const psc::PmModuleId dependencies[] = { psc::PmModuleId_Bluetooth }; //PscPmModuleId_Bluetooth, PscPmModuleId_Btm, PscPmModuleId_Hid ??
            R_ABORT_UNLESS(pmModule.Initialize(pmModuleId, dependencies, util::size(dependencies), os::EventClearMode_AutoClear));

            while (true) {
                /* Check power management events */
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

        void BluetoothEventThreadFunc(void *arg) {
            while (true) {
                // Wait for real bluetooth event 
                os::WaitSystemEvent(&g_btSystemEvent);

                BTDRV_LOG_FMT("bluetooth event fired");
                
                // Signal our forwarder events
                if (!g_redirectEvents || g_preparingForSleep)
                    os::SignalSystemEvent(&g_btSystemEventFwd);
                else
                    os::SignalSystemEvent(&g_btSystemEventUser);

            }
        }

    
        void handleConnectionStateEvent(HidEventData *eventData) {
            switch (eventData->connectionState.state) {
                case HidConnectionState_Connected:
                    attachDeviceHandler(&eventData->connectionState.address);
                    BTDRV_LOG_FMT("device connected");
                    break;
                case HidConnectionState_Disconnected:
                    removeDeviceHandler(&eventData->connectionState.address);
                    BTDRV_LOG_FMT("device disconnected");
                    break;
                default:
                    break;
            }
            BTDRV_LOG_DATA(&eventData->connectionState.address, sizeof(BluetoothAddress));
        }

        void BluetoothHidEventThreadFunc(void *arg) {
            HidEventData *eventData = reinterpret_cast<HidEventData *>(g_bt_hid_event_data_buffer);

            while (true) {
                // Wait for real bluetooth event 
                os::WaitSystemEvent(&g_btHidSystemEvent);

                BTDRV_LOG_FMT("hid event fired");

                R_ABORT_UNLESS(btdrvGetHidEventInfo(&g_current_bt_hid_event_type, g_bt_hid_event_data_buffer, sizeof(g_bt_hid_event_data_buffer)));
    
                switch (g_current_bt_hid_event_type) {
                    case HidEvent_ConnectionState:
                        handleConnectionStateEvent(eventData);
                        break;
                    default:
                        break;
                }
                     
                // Signal our forwarder events
                //os::SignalSystemEvent(&g_btHidSystemEventFwd);

                if (!g_redirectEvents || g_preparingForSleep) {
                    os::SignalSystemEvent(&g_btHidSystemEventFwd);
                }
                else {
                    os::SignalSystemEvent(&g_btHidSystemEventUser);
                }
            }
        }

        void BluetoothHidReportEventThreadFunc(void *arg) {
            /*
            R_ABORT_UNLESS(hiddbgInitialize());
            // Todo: move these to some class constuctor or something?
            if (hos::GetVersion() >= hos::Version_7_0_0)
                R_ABORT_UNLESS(hiddbgAttachHdlsWorkBuffer());
            */

            while (true) {
                // Wait for real bluetooth event 
                os::WaitSystemEvent(&g_btHidReportSystemEvent);

                ProcessHidReportPackets(g_realCircBuff, g_fakeCircBuff);

                // Signal our forwarder events
                //os::SignalSystemEvent(&btHidReportSystemEventUser);
                os::SignalSystemEvent(&g_btHidReportSystemEventFwd);

                //BTDRV_LOG_FMT("wrote hid report packets");
            }

            /*
            if (hos::GetVersion() >= hos::Version_7_0_0)
                R_ABORT_UNLESS(hiddbgReleaseHdlsWorkBuffer());
            
            hiddbgExit();
            */
        }

        void BluetoothBleEventThreadFunc(void *arg) {
            while (true) {
                // Wait for real bluetooth event 
                os::WaitSystemEvent(&g_btBleSystemEvent);

                BTDRV_LOG_FMT("ble event fired");
                
                // Signal our forwarder events
                if (!g_redirectEvents || g_preparingForSleep)
                    os::SignalSystemEvent(&g_btBleSystemEventFwd);
                else
                    os::SignalSystemEvent(&g_btBleSystemEventUser);
            }
        }

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
            R_ABORT_UNLESS(os::CreateSystemEvent(&g_btSystemEventFwd, os::EventClearMode_AutoClear, true));
            R_ABORT_UNLESS(os::CreateSystemEvent(&g_btSystemEventUser, os::EventClearMode_AutoClear, true));
            
            // Set callers handle to that of our forwarder event
            out_handle.SetValue(os::GetReadableHandleOfSystemEvent(&g_btSystemEventFwd)); 

            // Create and map fake bluetooth hid report shared memory
            R_ABORT_UNLESS(shmemCreate(&g_fakeBtShmem, BLUETOOTH_SHAREDMEM_SIZE, Perm_Rw, Perm_Rw));
            R_ABORT_UNLESS(shmemMap(&g_fakeBtShmem));
            g_fakeCircBuff = reinterpret_cast<bluetooth::CircularBuffer *>(shmemGetAddr(&g_fakeBtShmem));
            BTDRV_LOG_FMT("Fake shmem @ 0x%p", (void *)g_fakeCircBuff);

            // Initialise fake hid report buffer
            g_fakeCircBuff->Initialize("HID Report");
            g_fakeCircBuff->id = 1;
            g_fakeCircBuff->_unk3 = 1;

            // Create thread for forwarding events
            R_ABORT_UNLESS(os::CreateThread(&g_bt_event_task_thread, 
                BluetoothEventThreadFunc, 
                nullptr, 
                g_bt_event_task_stack, 
                sizeof(g_bt_event_task_stack), 
                9
                //37 // priority of btm sysmodule
            ));

            os::StartThread(&g_bt_event_task_thread); 

            g_bluetoothInitialized = true;

        } else {
            //out_handle.SetValue(os::GetReadableHandleOfSystemEvent(&g_btSystemEvent));
            //out_handle.SetValue(os::GetReadableHandleOfSystemEvent(&g_btSystemEventFwd));
            out_handle.SetValue(os::GetReadableHandleOfSystemEvent(&g_btSystemEventUser));
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


    Result BtdrvMitmService::CancelBond(BluetoothAddress address) {

        BTDRV_LOG_FMT("btdrv-mitm: CancelBond");

        R_ABORT_UNLESS(btdrvCancelBondFwd(this->forward_service.get(), &address));

        return ams::ResultSuccess();
    }



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
            R_ABORT_UNLESS(os::CreateSystemEvent(&g_btHidSystemEventFwd, os::EventClearMode_AutoClear, true));
            R_ABORT_UNLESS(os::CreateSystemEvent(&g_btHidSystemEventUser, os::EventClearMode_AutoClear, true));

            out_handle.SetValue(os::GetReadableHandleOfSystemEvent(&g_btHidSystemEventFwd)); 

             // Create thread for forwarding events
            R_ABORT_UNLESS(os::CreateThread(&g_bt_hid_event_task_thread, 
                BluetoothHidEventThreadFunc, 
                nullptr, 
                g_bt_hid_event_task_stack, 
                sizeof(g_bt_hid_event_task_stack), 
                9
                //38 // priority of btm sysmodule + 1
            ));

            os::StartThread(&g_bt_hid_event_task_thread); 

            g_hidInitialized = true;

        }
        else {
            BTDRV_LOG_FMT("btdrv-mitm: hid already initialized");
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

        if (this->client_info.program_id == ncm::SystemProgramId::Btm) {
            R_ABORT_UNLESS(btdrvFinalizeHidFwd(this->forward_service.get()));
        }

        return ams::ResultSuccess();
    }

    Result BtdrvMitmService::GetHidEventInfo(sf::Out<u32> out_type, const sf::OutPointerBuffer &out_buffer) {

        BTDRV_LOG_FMT("btdrv-mitm: GetHidEventInfo");

        
        if (this->client_info.program_id == ncm::SystemProgramId::Btm) {
            // Do we need to trick btm here?
        }

        //out_type.SetValue(g_current_bt_hid_event_type);
        //std::memcpy(out_buffer.GetPointer(), g_bt_hid_event_data_buffer, std::min(out_buffer.GetSize(), sizeof(g_bt_hid_event_data_buffer)));
        
        HidEventType event_type;

        R_ABORT_UNLESS(btdrvGetHidEventInfoFwd(this->forward_service.get(), 
            &event_type,
            static_cast<u8 *>(out_buffer.GetPointer()), 
            static_cast<size_t>(out_buffer.GetSize())
        ));

        out_type.SetValue(event_type);

        //BTDRV_LOG_FMT("  event %02d", event_type);
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
            R_ABORT_UNLESS(os::CreateSystemEvent(&g_btHidReportSystemEventFwd, os::EventClearMode_AutoClear, true));
            R_ABORT_UNLESS(os::CreateSystemEvent(&g_btHidReportSystemEventUser, os::EventClearMode_AutoClear, true));
           
            // Set callers handle to that of our forwarder event
            out_handle.SetValue(os::GetReadableHandleOfSystemEvent(&g_btHidReportSystemEventFwd));

            // Create thread for forwarding events
            R_ABORT_UNLESS(os::CreateThread(&g_bt_hid_report_event_task_thread, 
                BluetoothHidReportEventThreadFunc, 
                nullptr, 
                g_bt_hid_report_event_task_stack, 
                sizeof(g_bt_hid_report_event_task_stack), 
                -10
                //18  // priority of hid sysmodule
            ));

            os::StartThread(&g_bt_hid_report_event_task_thread); 

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

        if (!g_bluetoothInitialized || hos::GetVersion() < hos::Version_7_0_0) {
            // Todo: return error
        }

        Handle handle = INVALID_HANDLE;

        R_ABORT_UNLESS(btdrvGetHidReportEventInfoFwd(this->forward_service.get(), &handle));

        // Load and map the real bluetooth shared memory
        shmemLoadRemote(&g_realBtShmem, handle, BLUETOOTH_SHAREDMEM_SIZE, Perm_Rw);
        R_ABORT_UNLESS(shmemMap(&g_realBtShmem));
        g_realCircBuff = reinterpret_cast<bluetooth::CircularBuffer *>(shmemGetAddr(&g_realBtShmem));
        BTDRV_LOG_FMT("Real shmem @ 0x%p", (void *)g_realCircBuff);
       
        // Return the handle of our fake shared memory to the caller instead
        out_handle.SetValue(g_fakeBtShmem.handle);
        //out_handle.SetValue(handle);
        
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



    Result BtdrvMitmService::RedirectSystemEvents(bool redirect) {

        BTDRV_LOG_FMT("btdrv-mitm: RedirectSystemEvents");

        g_redirectEvents = redirect;

        return ams::ResultSuccess();
    }

}
