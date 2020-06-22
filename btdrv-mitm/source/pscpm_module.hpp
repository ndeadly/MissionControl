#pragma once
#include <switch.h>
#include <stratosphere.hpp>

namespace ams::mitm::btdrv {

    psc::PmModule *GetPscPmModule(void);
    Result InitializePscPmModule(void);
    void FinalizePscPmModule(void);
    Result StartPscPmThread(void);

    void HandlePscPmEvent(void);

}
