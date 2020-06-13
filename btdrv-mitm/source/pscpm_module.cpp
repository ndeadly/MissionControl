#include "pscpm_module.hpp"
#include "btdrv_mitm_flags.hpp"

#include "btdrv_mitm_logging.hpp"

namespace ams::mitm::btdrv {

    namespace {

        os::ThreadType g_eventHandlerThread;
        alignas(os::ThreadStackAlignment) u8 g_eventHandlerThreadStack[0x2000];

        psc::PmModule   g_pmModule;

    }

    void handlePscPmEvent(void) {
        psc::PmState    pmState;
        psc::PmFlagSet  pmFlags;

        if (R_SUCCEEDED(g_pmModule.GetRequest(&pmState, &pmFlags))) {
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

            R_ABORT_UNLESS(g_pmModule.Acknowledge(pmState, ams::ResultSuccess()));
        }
    }

    psc::PmModule *GetPscPmModule(void) {
        return &g_pmModule;
    }

    ams::Result InitializePscPmModule(void) {
        psc::PmModuleId pmModuleId = static_cast<psc::PmModuleId>(0xbd);
        const psc::PmModuleId dependencies[] = { psc::PmModuleId_Bluetooth }; //PscPmModuleId_Bluetooth, PscPmModuleId_Btm, PscPmModuleId_Hid ??
        R_TRY(g_pmModule.Initialize(pmModuleId, dependencies, util::size(dependencies), os::EventClearMode_AutoClear));
        
        return ams::ResultSuccess();
    }

    void FinalizePscPmModule(void) {
        g_pmModule.Finalize();
    }
            
    void PscMmThreadFunc(void *arg) {
        
        while (true) {
            // Check power management events
            g_pmModule.GetEventPointer()->Wait();

            handlePscPmEvent();
        }

        FinalizePscPmModule();
    }

    Result StartPscPmThread(void) {
        R_TRY(os::CreateThread(&g_eventHandlerThread, 
            PscMmThreadFunc, 
            nullptr, 
            g_eventHandlerThreadStack, 
            sizeof(g_eventHandlerThreadStack), 
            12
        ));

        os::StartThread(&g_eventHandlerThread); 

        return ams::ResultSuccess();
    }

}
