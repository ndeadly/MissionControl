/*
 * Copyright (c) 2020-2022 ndeadly
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

            R_ABORT_UNLESS(fs::MountSdCard("sdmc"));
        }

        void FinalizeSystemModule() { /* ... */ }

        void Startup() { /* ... */ }

    }

    void Main() {
        // Initialise module configuration
        mitm::InitializeConfig();

        // Start initialisation thread
        mitm::StartInitialize();

        // Launch mitm modules
        mitm::LaunchModules();

        // Wait for mitm modules to terminate
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
