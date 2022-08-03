/*
 * Copyright (c) 2020-2023 ndeadly
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <switch.h>
#include <stratosphere.hpp>
#include "mcmitm_initialization.hpp"
#include "mcmitm_config.hpp"
#include "mcmitm_process_monitor.hpp"

namespace ams {

    namespace mitm {

        namespace {

            alignas(0x40) constinit u8 g_heap_memory[64_KB];
            constinit lmem::HeapHandle g_heap_handle;
            constinit bool g_heap_initialized;
            constinit os::SdkMutex g_heap_init_mutex;

            lmem::HeapHandle GetHeapHandle() {
                if (AMS_UNLIKELY(!g_heap_initialized)) {
                    std::scoped_lock lk(g_heap_init_mutex);

                    if (AMS_LIKELY(!g_heap_initialized)) {
                        g_heap_handle = lmem::CreateExpHeap(g_heap_memory, sizeof(g_heap_memory), lmem::CreateOption_ThreadSafe);
                        g_heap_initialized = true;
                    }
                }

                return g_heap_handle;
            }

            void *Allocate(size_t size) {
                return lmem::AllocateFromExpHeap(GetHeapHandle(), size);
            }

            void *AllocateWithAlign(size_t size, size_t align) {
                return lmem::AllocateFromExpHeap(GetHeapHandle(), size, align);
            }

            void Deallocate(void *p, size_t size) {
                AMS_UNUSED(size);
                return lmem::FreeToExpHeap(GetHeapHandle(), p);
            }

        }

    }

    namespace init {

        void InitializeSystemModule() {
            R_ABORT_UNLESS(sm::Initialize());

            fs::InitializeForSystem();
            fs::SetAllocator(mitm::Allocate, mitm::Deallocate);
            fs::SetEnabledAutoAbort(false);

            R_ABORT_UNLESS(pmdmntInitialize());
            R_ABORT_UNLESS(pminfoInitialize());
            R_ABORT_UNLESS(pscmInitialize());
            R_ABORT_UNLESS(usbHsInitialize());

            R_ABORT_UNLESS(fs::MountSdCard("sdmc"));
        }

        void FinalizeSystemModule() { /* ... */ }

        void Startup() {
            // Load module configuration from ini file
            mitm::LoadConfiguration();
        }

    }

    void Main() {
        // Launch mitm and other modules
        mitm::LaunchModules();

        // Initialise pm module
        psc::PmModuleId pm_module_id = static_cast<psc::PmModuleId>(0xBD);
        const psc::PmModuleId pm_dependencies[] = { psc::PmModuleId_Fs };
        psc::PmModule pm_module;
        psc::PmState pm_state;
        psc::PmFlagSet pm_flags;
        R_ABORT_UNLESS(pm_module.Initialize(pm_module_id, pm_dependencies, util::size(pm_dependencies), os::EventClearMode_ManualClear));

        // Create timer event for periodically checking whether the current running application has changed
        os::TimerEvent timer_event(os::EventClearMode_ManualClear);
        timer_event.StartPeriodic(ams::TimeSpan::FromSeconds(1), ams::TimeSpan::FromSeconds(1));

        os::MultiWaitType wait_manager;
        os::MultiWaitHolderType holder_pm_module;
        os::MultiWaitHolderType holder_proc_monitor;

        os::InitializeMultiWait(&wait_manager);

        os::InitializeMultiWaitHolder(&holder_pm_module, pm_module.GetEventPointer()->GetBase());
        os::SetMultiWaitHolderUserData(&holder_pm_module, 0);
        os::LinkMultiWaitHolder(&wait_manager, &holder_pm_module);

        os::InitializeMultiWaitHolder(&holder_proc_monitor, timer_event.GetBase());
        os::SetMultiWaitHolderUserData(&holder_proc_monitor, 1);
        os::LinkMultiWaitHolder(&wait_manager, &holder_proc_monitor);

        // Loop events until shutdown signal is received
        bool shutdown = false;
        while (!shutdown) {
            auto signalled_holder = os::WaitAny(&wait_manager);
            switch (os::GetMultiWaitHolderUserData(signalled_holder)) {
                case 0:
                    pm_module.GetEventPointer()->Clear();
                    if (R_SUCCEEDED(pm_module.GetRequest(&pm_state, &pm_flags))) {
                        switch (pm_state) {
                            case psc::PmState_ShutdownReady:
                                shutdown = true;
                                [[fallthrough]];
                            case psc::PmState_SleepReady:
                                /* Run sleep/shutdown code */
                                break;
                            default:
                                break;
                        }
                    }

                    R_ABORT_UNLESS(pm_module.Acknowledge(pm_state, ResultSuccess()));
                    break;

                case 1:
                    timer_event.Clear();
                    mc::CheckForProcessSwitch();
                    break;

                AMS_UNREACHABLE_DEFAULT_CASE();
            }
        }

        // Stop the timer
        timer_event.Stop();

        // Wait for modules to terminate
        mitm::WaitModules();
    }

}

void *operator new(size_t size) {
    return ams::mitm::Allocate(size);
}

void *operator new(size_t size, const std::nothrow_t &) {
    return ams::mitm::Allocate(size);
}

void operator delete(void *p) {
    return ams::mitm::Deallocate(p, 0);
}

void operator delete(void *p, size_t size) {
    return ams::mitm::Deallocate(p, size);
}

void *operator new[](size_t size) {
    return ams::mitm::Allocate(size);
}

void *operator new[](size_t size, const std::nothrow_t &) {
    return ams::mitm::Allocate(size);
}

void operator delete[](void *p) {
    return ams::mitm::Deallocate(p, 0);
}

void operator delete[](void *p, size_t size) {
    return ams::mitm::Deallocate(p, size);
}

void *operator new(size_t size, std::align_val_t align) {
    return ams::mitm::AllocateWithAlign(size, static_cast<size_t>(align));
}

void operator delete(void *p, std::align_val_t align) {
    AMS_UNUSED(align);
    return ams::mitm::Deallocate(p, 0);
}
