#pragma once
#include "switchcontroller.hpp"

namespace ams::controller {

    class FakeSwitchController : public SwitchController {

        public:
            FakeSwitchController(ControllerType type, const bluetooth::Address *address) 
                : SwitchController(type, address) { };
            
            const bluetooth::HidReport * handleIncomingReport(const bluetooth::HidReport *report);
            const bluetooth::HidReport * handleOutgoingReport(const bluetooth::HidReport *report);

        protected:
            virtual Result setVibration(void);
            virtual Result setPlayerLed(u8 led_mask);

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

            bluetooth::HidReport m_inputReport;
            bluetooth::HidReport m_outputReport;
    };

}
