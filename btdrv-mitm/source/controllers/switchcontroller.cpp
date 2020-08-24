#include "switchcontroller.hpp"
#include "../bluetooth/bluetooth_hid_report.hpp"

namespace ams::controller {

    Result SwitchController::handleIncomingReport(const bluetooth::HidReport *report) {
        return bluetooth::hid::report::WriteHidReportBuffer(&m_address, report);
    }

    const bluetooth::HidReport * SwitchController::handleOutgoingReport(const bluetooth::HidReport *report) {
        return report;
    }

}
