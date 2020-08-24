#pragma once
#include "switchcontroller.hpp"

namespace ams::controller {

    inline bool bdcmp(const bluetooth::Address *addr1, const bluetooth::Address *addr2) {
        return std::memcmp(addr1, addr2, sizeof(bluetooth::Address)) == 0;
    }

    inline void packStickData(SwitchStickData *stick, uint16_t x, uint16_t y) {
        *stick = (SwitchStickData){
            static_cast<uint8_t>(x & 0xff), 
            static_cast<uint8_t>((x >> 8) | ((y & 0xff) << 4)), 
            static_cast<uint8_t>((y >> 4) & 0xff)
        };
    }

    class EmulatedSwitchController : public SwitchController {

        public:
            EmulatedSwitchController(ControllerType type, const bluetooth::Address *address) 
                : SwitchController(type, address)           
                , m_charging(false)
                , m_battery(BATTERY_MAX) { };
            
            Result handleIncomingReport(const bluetooth::HidReport *report);
            const bluetooth::HidReport * handleOutgoingReport(const bluetooth::HidReport *report);

        protected:
            virtual void convertReportFormat(const bluetooth::HidReport *inReport, bluetooth::HidReport *outReport) {};

            virtual Result setVibration(void) { return ams::ResultSuccess(); };
            virtual Result setPlayerLed(u8 led_mask) { return ams::ResultSuccess(); };

            Result handleSubCmdReport(const bluetooth::HidReport *report);
            Result subCmdRequestDeviceInfo(const bluetooth::HidReport *report);
            Result subCmdSpiFlashRead(const bluetooth::HidReport *report);
            Result subCmdSpiFlashWrite(const bluetooth::HidReport *report);
            Result subCmdSpiSectorErase(const bluetooth::HidReport *report);
            Result subCmdSetInputReportMode(const bluetooth::HidReport *report);
            Result subCmdTriggersElapsedTime(const bluetooth::HidReport *report);
            Result subCmdSetShipPowerState(const bluetooth::HidReport *report);
            Result subCmdSetMcuConfig(const bluetooth::HidReport *report);
            Result subCmdSetMcuState(const bluetooth::HidReport *report);
            Result subCmdSetPlayerLeds(const bluetooth::HidReport *report);
            Result subCmdEnableImu(const bluetooth::HidReport *report);
            Result subCmdEnableVibration(const bluetooth::HidReport *report);

            Result fakeSubCmdResponse(const u8 response[], size_t size);

            bool    m_charging;
            uint8_t m_battery;
            bluetooth::HidReport m_inputReport;
            bluetooth::HidReport m_outputReport;
    };

}
