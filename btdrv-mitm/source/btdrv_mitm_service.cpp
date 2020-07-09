#include <cstring>

#include <switch.h>
#include "btdrv_mitm_service.hpp"
#include "btdrv_mitm_flags.hpp"
#include "btdrv_shim.h"

#include "bluetooth/bluetooth_events.hpp"
#include "controllermanager.hpp"

namespace ams::mitm::btdrv {

    Result BtdrvMitmService::InitializeBluetooth(sf::OutCopyHandle out_handle) {

        BTDRV_LOG_FMT("btdrv-mitm: InitializeBluetooth");

        if (!bluetooth::core::IsInitialized()) {
            Handle handle = INVALID_HANDLE;
            R_TRY(btdrvInitializeBluetoothFwd(this->forward_service.get(), &handle));
            R_TRY(bluetooth::core::Initialize(handle));
            R_TRY(bluetooth::hid::report::InitializeReportBuffer());           

            //if (hos::GetVersion() >= hos::Version_7_0_0)
                //R_TRY(bluetooth::hid::report::InitializeFakeSharedMemory());

            out_handle.SetValue(os::GetReadableHandleOfSystemEvent(bluetooth::core::GetForwardEvent())); 
        } else {
            out_handle.SetValue(os::GetReadableHandleOfSystemEvent(bluetooth::core::GetUserForwardEvent()));
        }

        return ams::ResultSuccess();
    }

    Result BtdrvMitmService::FinalizeBluetooth(void) {

        BTDRV_LOG_FMT("btdrv-mitm: FinalizeBluetooth");

        // Only btm should be able to make this call
        if (this->client_info.program_id == ncm::SystemProgramId::Btm) {
            R_TRY(btdrvFinalizeBluetoothFwd(this->forward_service.get()));
        }

        return ams::ResultSuccess();
    }

    Result BtdrvMitmService::GetEventInfo(sf::Out<bluetooth::EventType> out_type, const sf::OutPointerBuffer &out_buffer) {

        BTDRV_LOG_FMT("btdrv-mitm: GetEventInfo");

        R_TRY(bluetooth::core::GetEventInfo(this->client_info.program_id,
            out_type.GetPointer(),
            static_cast<u8 *>(out_buffer.GetPointer()), 
            static_cast<size_t>(out_buffer.GetSize())
        ));

        return ams::ResultSuccess();
    }

    Result BtdrvMitmService::InitializeHid(sf::OutCopyHandle out_handle, u16 version) {

        BTDRV_LOG_FMT("btdrv-mitm: InitializeHid");

        if (!bluetooth::hid::IsInitialized()) {
            Handle handle = INVALID_HANDLE;
            R_TRY(btdrvInitializeHidFwd(this->forward_service.get(), &handle, version));
            R_TRY(bluetooth::hid::Initialize(handle));

            out_handle.SetValue(os::GetReadableHandleOfSystemEvent(bluetooth::hid::GetForwardEvent())); 
        }
        else {
            out_handle.SetValue(os::GetReadableHandleOfSystemEvent(bluetooth::hid::GetUserForwardEvent()));
        }

        return ams::ResultSuccess();
    }

    Result BtdrvMitmService::WriteHidData(bluetooth::Address address, const sf::InPointerBuffer &buffer) {

        BTDRV_LOG_FMT("btdrv-mitm: WriteHidData");

        if (this->client_info.program_id == ncm::SystemProgramId::Hid) {
            auto controller = locateController(&address);
            if (controller && !controller->isSwitchController()) {

                // TODO: convert hid data format where possible and call btdrvWriteHidDataFwd
                auto requestData = reinterpret_cast<const bluetooth::HidData *>(buffer.GetPointer());
                u8 cmdId = requestData->data[0];
                
                if (cmdId == 0x01) {
                    auto subCmdId = static_cast<bluetooth::SubCmdType>(requestData->data[10]);
                    BTDRV_LOG_FMT("Subcommand report [%02x]", subCmdId);

                    switch (subCmdId) {
                        case bluetooth::SubCmd_RequestDeviceInfo:
                            {
                                s64 timer = os::ConvertToTimeSpan(os::GetSystemTick()).GetMilliSeconds();
                                u8 reportData[] = {0x31, 0x00, 0x21, timer & 0xff, 0x80, 0x00, 0x00, 0x00, 0x0b, 0xb8, 0x78, 0xd9, 0xd7, 0x81, 0x00,
                                                0x82, 0x02, 0x03, 0x48, 0x03, 0x02, address.address[0], address.address[1], address.address[2], address.address[3], address.address[4], address.address[5], 0x01, 0x02};
                                
                                auto responseData = reinterpret_cast<bluetooth::HidData *>(reportData);
                                bluetooth::hid::report::WriteFakeHidData(&address, responseData);
                            }
                            break;

                        case bluetooth::SubCmd_SpiFlashRead:
                            {
                                // Official controller reads these
                                // @ 0x00006000: ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff  <= Serial 
                                // @ 0x00006050: 32 32 32 ff ff ff ff ff ff ff ff ff  <= RGB colours (body, buttons, left grip, right grip)
                                // @ 0x00006080: 50 fd 00 00 c6 0f 0f 30 61 ae 90 d9 d4 14 54 41 15 54 c7 79 9c 33 36 63  <= Factory Sensor and Stick device parameters
                                // @ 0x00006098: 0f 30 61 ae 90 d9 d4 14 54 41 15 54 c7 79 9c 33 36 63  <= Stick device parameters 2. Normally the same with 1, even in Pro Contr.
                                // @ 0x00008010: ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff
                                // @ 0x0000603d: e6 a5 67 1a 58 78 50 56 60 1a f8 7f 20 c6 63 d5 15 5e ff 32 32 32 ff ff ff  <= Left analog stick calibration
                                // @ 0x00006020: 64 ff 33 00 b8 01 00 40 00 40 00 40 17 00 d7 ff bd ff 3b 34 3b 34 3b 34  <= 6-Axis motion sensor Factory calibration
                                
                                u32 read_addr = *(u32 *)(&requestData->data[11]);
                                u8  read_size = requestData->data[15];
                                BTDRV_LOG_DATA_MSG((void *)requestData, requestData->length+2, "SPI flash read: %d bytes @ 0x%08x", read_size, read_addr);

                                s64 timer = os::ConvertToTimeSpan(os::GetSystemTick()).GetMilliSeconds();

                                if (read_addr == 0x6000 && read_size == 0x10) {
                                    u8 reportData[] = {0x31, 0x00, 0x21, timer & 0xff, 0x80, 0x00, 0x00, 0x00, 0x0b, 0xb8, 0x78, 0xd9, 0xd7, 0x81, 0x00,
                                                    0x90, subCmdId, requestData->data[11], requestData->data[12], requestData->data[13], requestData->data[14], requestData->data[15],
                                                    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

                                    auto responseData = reinterpret_cast<bluetooth::HidData *>(reportData);
                                    bluetooth::hid::report::WriteFakeHidData(&address, responseData);
                                }

                                else if (read_addr == 0x6050 && read_size == 0x0d) {
                                    u8 reportData[] = {0x31, 0x00, 0x21, timer & 0xff, 0x80, 0x00, 0x00, 0x00, 0x0b, 0xb8, 0x78, 0xd9, 0xd7, 0x81, 0x00,
                                                    0x90, subCmdId, requestData->data[11], requestData->data[12], requestData->data[13], requestData->data[14], requestData->data[15],
                                                    //0x32, 0x32, 0x32, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
                                                    0x32, 0x32, 0x32, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0xff, 0x00, 0x00
                                                    };

                                    auto responseData = reinterpret_cast<bluetooth::HidData *>(reportData);
                                    bluetooth::hid::report::WriteFakeHidData(&address, responseData);
                                }

                                else if (read_addr == 0x6080 && read_size == 0x18) {
                                    u8 reportData[] = {0x31, 0x00, 0x21, timer & 0xff, 0x80, 0x00, 0x00, 0x00, 0x0b, 0xb8, 0x78, 0xd9, 0xd7, 0x81, 0x00,
                                                    0x90, subCmdId, requestData->data[11], requestData->data[12], requestData->data[13], requestData->data[14], requestData->data[15],
                                                    0x50, 0xfd, 0x00, 0x00, 0xc6, 0x0f, 0x0f, 0x30, 0x61, 0xae, 0x90, 0xd9, 0xd4, 0x14, 0x54, 0x41, 0x15, 0x54, 0xc7, 0x79, 0x9c, 0x33, 0x36, 0x63};

                                    auto responseData = reinterpret_cast<bluetooth::HidData *>(reportData);
                                    bluetooth::hid::report::WriteFakeHidData(&address, responseData);
                                }

                                else if (read_addr == 0x6098 && read_size == 0x12) {
                                    u8 reportData[] = {0x31, 0x00, 0x21, timer & 0xff, 0x80, 0x00, 0x00, 0x00, 0x0b, 0xb8, 0x78, 0xd9, 0xd7, 0x81, 0x00,
                                                    0x90, subCmdId, requestData->data[11], requestData->data[12], requestData->data[13], requestData->data[14], requestData->data[15],
                                                    0x0f, 0x30, 0x61, 0xae, 0x90, 0xd9, 0xd4, 0x14, 0x54, 0x41, 0x15, 0x54, 0xc7, 0x79, 0x9c, 0x33, 0x36, 0x63};

                                    auto responseData = reinterpret_cast<bluetooth::HidData *>(reportData);
                                    bluetooth::hid::report::WriteFakeHidData(&address, responseData);
                                }

                                else if (read_addr == 0x8010 && read_size == 0x18) {
                                    u8 reportData[] = {0x31, 0x00, 0x21, timer & 0xff, 0x80, 0x00, 0x00, 0x00, 0x0b, 0xb8, 0x78, 0xd9, 0xd7, 0x81, 0x00,
                                                    0x90, subCmdId, requestData->data[11], requestData->data[12], requestData->data[13], requestData->data[14], requestData->data[15],
                                                    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

                                    auto responseData = reinterpret_cast<bluetooth::HidData *>(reportData);
                                    bluetooth::hid::report::WriteFakeHidData(&address, responseData);
                                }

                                else if (read_addr == 0x603d && read_size == 0x19) {
                                    u8 reportData[] = {0x31, 0x00, 0x21, timer & 0xff, 0x80, 0x00, 0x00, 0x00, 0x0b, 0xb8, 0x78, 0xd9, 0xd7, 0x81, 0x00,
                                                    0x90, subCmdId, requestData->data[11], requestData->data[12], requestData->data[13], requestData->data[14], requestData->data[15],
                                                    0xe6, 0xa5, 0x67, 0x1a, 0x58, 0x78, 0x50, 0x56, 0x60, 0x1a, 0xf8, 0x7f, 0x20, 0xc6, 0x63, 0xd5, 0x15, 0x5e, 0xff, 0x32, 0x32, 0x32, 0xff, 0xff, 0xff};

                                    auto responseData = reinterpret_cast<bluetooth::HidData *>(reportData);
                                    bluetooth::hid::report::WriteFakeHidData(&address, responseData);
                                }

                                else if (read_addr == 0x6020 && read_size == 0x18) {
                                    u8 reportData[] = {0x31, 0x00, 0x21, timer & 0xff, 0x80, 0x00, 0x00, 0x00, 0x0b, 0xb8, 0x78, 0xd9, 0xd7, 0x81, 0x00,
                                                    0x90, subCmdId, requestData->data[11], requestData->data[12], requestData->data[13], requestData->data[14], requestData->data[15],
                                                    0x64, 0xff, 0x33, 0x00, 0xb8, 0x01, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x17, 0x00, 0xd7, 0xff, 0xbd, 0xff, 0x3b, 0x34, 0x3b, 0x34, 0x3b, 0x34};

                                    auto responseData = reinterpret_cast<bluetooth::HidData *>(reportData);
                                    bluetooth::hid::report::WriteFakeHidData(&address, responseData);
                                }


                            }
                            break;

                        case bluetooth::SubCmd_SetInputReportMode:
                            {
                                s64 timer = os::ConvertToTimeSpan(os::GetSystemTick()).GetMilliSeconds();
                                u8 reportData[] = {0x31, 0x00, 0x21, timer & 0xff, 0x80, 0x00, 0x00, 0x00, 0x0b, 0xb8, 0x78, 0xd9, 0xd7, 0x81, 0x00,
                                                0x80, subCmdId};

                                auto responseData = reinterpret_cast<bluetooth::HidData *>(reportData);
                                bluetooth::hid::report::WriteFakeHidData(&address, responseData);
                            }
                            break;

                        case bluetooth::SubCmd_TriggersElapsedTime:
                            {
                                s64 timer = os::ConvertToTimeSpan(os::GetSystemTick()).GetMilliSeconds();
                                u8 reportData[] = {0x31, 0x00, 0x21, timer & 0xff, 0x80, 0x00, 0x00, 0x00, 0x0b, 0xb8, 0x78, 0xd9, 0xd7, 0x81, 0x00,
                                                0x83, subCmdId};

                                auto responseData = reinterpret_cast<bluetooth::HidData *>(reportData);
                                bluetooth::hid::report::WriteFakeHidData(&address, responseData);
                            }
                            break;

                        case bluetooth::SubCmd_SetShipPowerState:
                            {
                                s64 timer = os::ConvertToTimeSpan(os::GetSystemTick()).GetMilliSeconds();
                                u8 reportData[] = {0x31, 0x00, 0x21, timer & 0xff, 0x80, 0x00, 0x00, 0x00, 0x0b, 0xb8, 0x78, 0xd9, 0xd7, 0x81, 0x00,
                                                0x80, subCmdId, 0x00};

                                auto responseData = reinterpret_cast<bluetooth::HidData *>(reportData);
                                bluetooth::hid::report::WriteFakeHidData(&address, responseData);
                            }
                            break;

                        case bluetooth::SubCmd_SetMcuConfig:
                            {
                                s64 timer = os::ConvertToTimeSpan(os::GetSystemTick()).GetMilliSeconds();
                                u8 reportData[] = {0x31, 0x00, 0x21, timer & 0xff, 0x80, 0x00, 0x00, 0x00, 0x0b, 0xb8, 0x78, 0xd9, 0xd7, 0x81, 0x00,
                                                0xa0, subCmdId, 0x01, 0x00, 0xff, 0x00, 0x03, 0x00, 0x05, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5c};

                                auto responseData = reinterpret_cast<bluetooth::HidData *>(reportData);
                                bluetooth::hid::report::WriteFakeHidData(&address, responseData);
                            }
                            break;

                        case bluetooth::SubCmd_SetMcuState:
                            {
                                //BTDRV_LOG_DATA_MSG((void *)requestData, requestData->length+2, "Set MCU State");

                                s64 timer = os::ConvertToTimeSpan(os::GetSystemTick()).GetMilliSeconds();
                                u8 reportData[] = {0x31, 0x00, 0x21, timer & 0xff, 0x80, 0x00, 0x00, 0x00, 0x0b, 0xb8, 0x78, 0xd9, 0xd7, 0x81, 0x00,
                                                0x80, subCmdId};

                                auto responseData = reinterpret_cast<bluetooth::HidData *>(reportData);
                                bluetooth::hid::report::WriteFakeHidData(&address, responseData);
                            }
                            break;


                        case bluetooth::SubCmd_SetPlayerLeds:
                            {
                                s64 timer = os::ConvertToTimeSpan(os::GetSystemTick()).GetMilliSeconds();
                                u8 reportData[] = {0x31, 0x00, 0x21, timer & 0xff, 0x80, 0x00, 0x00, 0x00, 0x0b, 0xb8, 0x78, 0xd9, 0xd7, 0x81, 0x00,
                                                0x80, subCmdId};

                                auto responseData = reinterpret_cast<bluetooth::HidData *>(reportData);
                                bluetooth::hid::report::WriteFakeHidData(&address, responseData);
                            }
                            break;
                        
                        case bluetooth::SubCmd_EnableImu:
                            {
                                s64 timer = os::ConvertToTimeSpan(os::GetSystemTick()).GetMilliSeconds();
                                u8 reportData[] = {0x31, 0x00, 0x21, timer & 0xff, 0x80, 0x00, 0x00, 0x00, 0x0b, 0xb8, 0x78, 0xd9, 0xd7, 0x81, 0x00,
                                                0x80, subCmdId};

                                auto responseData = reinterpret_cast<bluetooth::HidData *>(reportData);
                                bluetooth::hid::report::WriteFakeHidData(&address, responseData);
                            }
                            break;

                        case bluetooth::SubCmd_EnableVibration:
                            {
                                s64 timer = os::ConvertToTimeSpan(os::GetSystemTick()).GetMilliSeconds();
                                u8 reportData[] = {0x31, 0x00, 0x21, timer & 0xff, 0x80, 0x00, 0x00, 0x00, 0x0b, 0xb8, 0x78, 0xd9, 0xd7, 0x81, 0x00,
                                                0x80, subCmdId};

                                auto responseData = reinterpret_cast<bluetooth::HidData *>(reportData);
                                bluetooth::hid::report::WriteFakeHidData(&address, responseData);
                            }
                            break;

                        default:
                            break;
                    }
                }
                else if (cmdId == 0x10) {
                    // Rumble report
                }

                return ams::ResultSuccess();
            }
        }

        R_TRY(btdrvWriteHidDataFwd(this->forward_service.get(), 
            &address,
            reinterpret_cast<const bluetooth::HidData *>(buffer.GetPointer()) 
        ));

        return ams::ResultSuccess();
    }

    /*
    Result BtdrvMitmService::SetHidReport(bluetooth::Address address, bluetooth::HhReportType type, const sf::InPointerBuffer &buffer) {

        BTDRV_LOG_FMT("btdrv-mitm: SetHidReport");

        R_TRY(btdrvSetHidReportFwd(this->forward_service.get(), 
            &address, 
            type, 
            reinterpret_cast<const bluetooth::HidData *>(buffer.GetPointer())
        ));

        return ams::ResultSuccess();
    }

    Result BtdrvMitmService::GetHidReport(bluetooth::Address address, bluetooth::HhReportType type, u8 id) {

        BTDRV_LOG_FMT("btdrv-mitm: GetHidReport");

        R_TRY(btdrvGetHidReportFwd(this->forward_service.get(), &address, type, id));

        return ams::ResultSuccess();
    }
    */

    Result BtdrvMitmService::GetPairedDeviceInfo(sf::Out<bluetooth::DeviceSettings> out, bluetooth::Address address) {

        BTDRV_LOG_FMT("btdrv-mitm: GetPairedDeviceInfo");

        auto device = reinterpret_cast<BluetoothDevicesSettings *>(out.GetPointer());

        R_TRY(btdrvGetPairedDeviceInfoFwd(this->forward_service.get(), &address, device));

        if (this->client_info.program_id == ncm::SystemProgramId::Btm) {
            
            if (!IsValidSwitchControllerName(device->name)) {
                std::strncpy(device->name, "Lic Pro Controller", sizeof(BluetoothLocalName) - 1);
                device->device_class = {0x00, 0x25, 0x08};
            }

        }

        BTDRV_LOG_FMT("name: %s\nvid: %04x\npid: %04x", device->name, device->vid, device->pid);

        return ams::ResultSuccess();
    }
        
    Result BtdrvMitmService::FinalizeHid(void) {

        BTDRV_LOG_FMT("btdrv-mitm: FinalizeHid");

        // Only btm should be able to make this call
        if (this->client_info.program_id == ncm::SystemProgramId::Btm) {
            R_TRY(btdrvFinalizeHidFwd(this->forward_service.get()));
        }

        return ams::ResultSuccess();
    }

    Result BtdrvMitmService::GetHidEventInfo(sf::Out<bluetooth::HidEventType> out_type, const sf::OutPointerBuffer &out_buffer) {

        BTDRV_LOG_FMT("btdrv-mitm: GetHidEventInfo");

        R_TRY(bluetooth::hid::GetEventInfo(this->client_info.program_id,
            out_type.GetPointer(), 
            static_cast<u8 *>(out_buffer.GetPointer()),
            static_cast<size_t>(out_buffer.GetSize())
        ));

        return ams::ResultSuccess();
    }

    /* 1.0.0 - 3.0.2 */
    Result BtdrvMitmService::RegisterHidReportEventDeprecated(sf::OutCopyHandle out_handle) {
        return RegisterHidReportEvent(out_handle);
    }

    /* 4.0.0+ */
    Result BtdrvMitmService::RegisterHidReportEvent(sf::OutCopyHandle out_handle) {

        BTDRV_LOG_FMT("btdrv-mitm: RegisterHidReportEvent");

        if (!bluetooth::hid::report::IsInitialized()) {
            Handle handle = INVALID_HANDLE;
            R_TRY(btdrvRegisterHidReportEventFwd(this->forward_service.get(), &handle));
            R_TRY(bluetooth::hid::report::Initialize(handle));
            out_handle.SetValue(os::GetReadableHandleOfSystemEvent(bluetooth::hid::report::GetForwardEvent()));
        }
        else {
            out_handle.SetValue(os::GetReadableHandleOfSystemEvent(bluetooth::hid::report::GetUserForwardEvent()));
        }

        return ams::ResultSuccess();
    }

    /* 1.0.0 - 6.2.0 */
    Result _GetHidReportEventInfoDeprecated(Service *srv, sf::Out<bluetooth::HidEventType> out_type, const sf::OutPointerBuffer &out_buffer) {

        BTDRV_LOG_FMT("btdrv-mitm: GetHidReportEventInfo");

        R_TRY(bluetooth::hid::report::GetEventInfo(out_type.GetPointer(), 
            static_cast<u8 *>(out_buffer.GetPointer()),
            static_cast<size_t>(out_buffer.GetSize())
        ));

        return ams::ResultSuccess();
    }

    /* 1.0.0 - 3.0.2 */
    Result BtdrvMitmService::GetHidReportEventInfoDeprecated1(sf::Out<bluetooth::HidEventType> out_type, const sf::OutPointerBuffer &out_buffer) {
        return _GetHidReportEventInfoDeprecated(this->forward_service.get(), out_type, out_buffer);
    }

    /* 4.0.0 - 6.2.0 */
    Result BtdrvMitmService::GetHidReportEventInfoDeprecated2(sf::Out<bluetooth::HidEventType> out_type, const sf::OutPointerBuffer &out_buffer) {
        return _GetHidReportEventInfoDeprecated(this->forward_service.get(), out_type, out_buffer);
    }

    /* 7.0.0+ */
    Result BtdrvMitmService::GetHidReportEventInfo(sf::OutCopyHandle out_handle) {

        BTDRV_LOG_FMT("btdrv-mitm: GetHidReportEventInfo");

        Handle handle = INVALID_HANDLE;
        R_TRY(btdrvGetHidReportEventInfoFwd(this->forward_service.get(), &handle));
        R_TRY(bluetooth::hid::report::MapRemoteSharedMemory(handle));
        out_handle.SetValue(bluetooth::hid::report::GetFakeSharedMemory()->handle);
        
        return ams::ResultSuccess();
    }

    Result BtdrvMitmService::InitializeBle(sf::OutCopyHandle out_handle) {

        BTDRV_LOG_FMT("btdrv-mitm: InitializeBle");

        if (!bluetooth::ble::IsInitialized()) {
            Handle handle = INVALID_HANDLE;
            R_TRY(btdrvInitializeBleFwd(this->forward_service.get(), &handle));
            R_TRY(bluetooth::ble::Initialize(handle));

            out_handle.SetValue(os::GetReadableHandleOfSystemEvent(bluetooth::ble::GetForwardEvent())); 
        }
        else {
            out_handle.SetValue(os::GetReadableHandleOfSystemEvent(bluetooth::ble::GetUserForwardEvent()));
        }

        return ams::ResultSuccess();
    }

    Result BtdrvMitmService::FinalizeBle(void) {

        BTDRV_LOG_FMT("btdrv-mitm: FinalizeBle");

        if (this->client_info.program_id == ncm::SystemProgramId::Btm) {
            R_TRY(btdrvFinalizeBleFwd(this->forward_service.get()));
        }

        return ams::ResultSuccess();
    }

    Result BtdrvMitmService::GetBleManagedEventInfoDeprecated(sf::Out<bluetooth::BleEventType> out_type, const sf::OutPointerBuffer &out_buffer) {
        return GetBleManagedEventInfo(out_type, out_buffer);
    }
    
    Result BtdrvMitmService::GetBleManagedEventInfo(sf::Out<bluetooth::BleEventType> out_type, const sf::OutPointerBuffer &out_buffer) {
        
        BTDRV_LOG_FMT("btdrv-mitm: GetBleManagedEventInfo");
        
        R_TRY(bluetooth::ble::GetEventInfo(this->client_info.program_id,
            out_type.GetPointer(), 
            static_cast<u8 *>(out_buffer.GetPointer()),
            static_cast<size_t>(out_buffer.GetSize())
        ));

        return ams::ResultSuccess();
    }

    void BtdrvMitmService::RedirectSystemEvents(bool redirect) {

        BTDRV_LOG_FMT("btdrv-mitm: RedirectSystemEvents [%s]", redirect ? "on" : "off");

        g_redirectEvents = redirect;
    }

    void BtdrvMitmService::RedirectHidReportEvents(bool redirect) {

        BTDRV_LOG_FMT("btdrv-mitm: RedirectHidReportEvents [%s]", redirect ? "on" : "off");

        g_redirectHidReportEvents = redirect;
    }



}
