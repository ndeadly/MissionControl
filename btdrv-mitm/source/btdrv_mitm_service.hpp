#pragma once
#include <stratosphere.hpp>

#include "btdrv_mitm_logging.hpp"

namespace ams::mitm::btdrv {

    class BtdrvMitmService : public sf::IMitmServiceObject {

        private:
            enum class CommandId {
                InitializeBluetooth                 = 1,
                FinalizeBluetooth                   = 4,
                //CancelBond                          = 12,
                GetEventInfo                        = 15,
                InitializeHid                       = 16,
                WriteHidData                        = 19,
                FinalizeHid                         = 26,
                GetHidEventInfo                     = 27,
                RegisterHidReportEventDeprecated    = 36,
                RegisterHidReportEvent              = 37,
                GetHidReportEventInfoDeprecated1    = 37,
                GetHidReportEventInfoDeprecated2    = 38,
                GetHidReportEventInfo               = 38,
                /* 5.0.0+ */
                InitializeBle                       = 46,
                FinalizeBle                         = 49,
                GetBleManagedEventInfoDeprecated    = 78,
                GetBleManagedEventInfo              = 79,

                /* Extensions */
                RedirectSystemEvents                = 65000,
            };

        public:
            static bool ShouldMitm(const sm::MitmProcessInfo &client_info) {
                return true;
            }

        public:
            SF_MITM_SERVICE_OBJECT_CTOR(BtdrvMitmService) { }

        protected:
            Result InitializeBluetooth(sf::OutCopyHandle out_handle);
            Result FinalizeBluetooth(void);
            //Result CancelBond(BluetoothAddress address);
            Result GetEventInfo(sf::Out<BluetoothEventType> out_type, const sf::OutPointerBuffer &out_buffer);
            Result InitializeHid(sf::OutCopyHandle out_handle, u16 version);
            Result WriteHidData(BluetoothAddress address, const sf::InPointerBuffer &buffer);
            Result FinalizeHid(void);
            Result GetHidEventInfo(sf::Out<HidEventType> out_type, const sf::OutPointerBuffer &out_buffer);
            Result RegisterHidReportEventDeprecated(sf::OutCopyHandle out_handle);
            Result RegisterHidReportEvent(sf::OutCopyHandle out_handle);
            Result GetHidReportEventInfoDeprecated1(sf::Out<HidEventType> out_type, const sf::OutPointerBuffer &out_buffer);
            Result GetHidReportEventInfoDeprecated2(sf::Out<HidEventType> out_type, const sf::OutPointerBuffer &out_buffer);
            Result GetHidReportEventInfo(sf::OutCopyHandle out_handle);
            Result InitializeBle(sf::OutCopyHandle out_handle);
            Result FinalizeBle(void);
            Result GetBleManagedEventInfoDeprecated(sf::Out<BleEventType> out_type, const sf::OutPointerBuffer &out_buffer);
            Result GetBleManagedEventInfo(sf::Out<BleEventType> out_type, const sf::OutPointerBuffer &out_buffer);

            void RedirectSystemEvents(bool redirect);

        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(InitializeBluetooth),
                MAKE_SERVICE_COMMAND_META(FinalizeBluetooth),
                //MAKE_SERVICE_COMMAND_META(CancelBond),
                MAKE_SERVICE_COMMAND_META(GetEventInfo),
                MAKE_SERVICE_COMMAND_META(InitializeHid),
                MAKE_SERVICE_COMMAND_META(WriteHidData),
                MAKE_SERVICE_COMMAND_META(FinalizeHid),
                MAKE_SERVICE_COMMAND_META(GetHidEventInfo),
                MAKE_SERVICE_COMMAND_META(RegisterHidReportEventDeprecated, hos::Version_1_0_0, hos::Version_3_0_2),
                MAKE_SERVICE_COMMAND_META(RegisterHidReportEvent,           hos::Version_4_0_0),
                MAKE_SERVICE_COMMAND_META(GetHidReportEventInfoDeprecated1, hos::Version_1_0_0, hos::Version_3_0_2),
                MAKE_SERVICE_COMMAND_META(GetHidReportEventInfoDeprecated2, hos::Version_4_0_0, hos::Version_6_2_0),
                MAKE_SERVICE_COMMAND_META(GetHidReportEventInfo,            hos::Version_7_0_0),
                MAKE_SERVICE_COMMAND_META(InitializeBle,                    hos::Version_5_0_0),
                MAKE_SERVICE_COMMAND_META(FinalizeBle,                      hos::Version_5_0_0),
                MAKE_SERVICE_COMMAND_META(GetBleManagedEventInfoDeprecated, hos::Version_5_0_0, hos::Version_5_0_2),
                MAKE_SERVICE_COMMAND_META(GetBleManagedEventInfo,           hos::Version_5_1_0),

                MAKE_SERVICE_COMMAND_META(RedirectSystemEvents),
            };

    };

}
