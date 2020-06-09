#pragma once
#include <stratosphere.hpp>
#include "btdrv_mitm_logging.hpp"


namespace ams::mitm::btdrv {

    class BtdrvMitmService : public sf::IMitmServiceObject {

        private:
            enum class CommandId {
                InitializeBluetooth     = 1,
                FinalizeBluetooth       = 4,

                CancelBond              = 12,

                GetEventInfo            = 15,
                InitializeHid           = 16,
                WriteHidData            = 19,
                FinalizeHid             = 26,
                GetHidEventInfo         = 27,
                RegisterHidReportEvent  = 37,
                GetHidReportEventInfo   = 38,

                /* 5.0.0+ */
                InitializeBle           = 46,
                FinalizeBle             = 49,

                /* Extensions */
                RedirectSystemEvents       = 65000,
            };

        public:
            static bool ShouldMitm(const sm::MitmProcessInfo &client_info) {
                return true;
            }

        public:
            SF_MITM_SERVICE_OBJECT_CTOR(BtdrvMitmService) {
                //BTDRV_LOG_FMT("\nbtdrv-mitm initialised");
            }

        protected:
            Result InitializeBluetooth(sf::OutCopyHandle out_handle);
            Result FinalizeBluetooth(void);

            Result CancelBond(BluetoothAddress address);

            Result GetEventInfo(sf::Out<u32> out_type, const sf::OutPointerBuffer &out_buffer);
            Result InitializeHid(sf::OutCopyHandle out_handle, u16 version);
            Result WriteHidData(BluetoothAddress address, const sf::InPointerBuffer &buffer);
            Result FinalizeHid(void);
            Result GetHidEventInfo(sf::Out<u32> out_type, const sf::OutPointerBuffer &out_buffer);
            Result RegisterHidReportEvent(sf::OutCopyHandle out_handle);
            Result GetHidReportEventInfo(sf::OutCopyHandle out_handle);

            Result InitializeBle(sf::OutCopyHandle out_handle);
            Result FinalizeBle(void);

            Result RedirectSystemEvents(bool redirect);

        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(InitializeBluetooth),
                MAKE_SERVICE_COMMAND_META(FinalizeBluetooth),

                MAKE_SERVICE_COMMAND_META(CancelBond),

                MAKE_SERVICE_COMMAND_META(GetEventInfo),
                MAKE_SERVICE_COMMAND_META(InitializeHid),
                MAKE_SERVICE_COMMAND_META(WriteHidData),
                MAKE_SERVICE_COMMAND_META(FinalizeHid),
                MAKE_SERVICE_COMMAND_META(GetHidEventInfo),
                MAKE_SERVICE_COMMAND_META(RegisterHidReportEvent),
                MAKE_SERVICE_COMMAND_META(GetHidReportEventInfo),

                MAKE_SERVICE_COMMAND_META(InitializeBle),
                MAKE_SERVICE_COMMAND_META(FinalizeBle),

                MAKE_SERVICE_COMMAND_META(RedirectSystemEvents),
            };

    };

}
