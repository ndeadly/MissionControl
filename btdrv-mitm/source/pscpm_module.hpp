#pragma once
#include <switch.h>
#include <stratosphere.hpp>

namespace ams::mitm::btdrv {

    void handlePscPmEvent(void);

    psc::PmModule *GetPscPmModule(void);
    Result InitializePscPmModule(void);
    void FinalizePscPmModule(void);
    Result StartPscPmThread(void);

}
