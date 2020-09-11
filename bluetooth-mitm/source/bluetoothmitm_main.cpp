/*
 * Copyright (C) 2020 ndeadly
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <switch.h>
#include <stratosphere.hpp>
#include "btdrv_mitm/btdrvmitm_module.hpp"
#include "btm_mitm/btmmitm_module.hpp"

extern "C" {

    extern u32 __start__;

    u32 __nx_applet_type = AppletType_None;
    u32 __nx_fs_num_sessions = 1;

    #define INNER_HEAP_SIZE 0x10000
    size_t nx_inner_heap_size = INNER_HEAP_SIZE;
    char   nx_inner_heap[INNER_HEAP_SIZE];

    void __libnx_initheap(void);
    void __appInit(void);
    void __appExit(void);

    /* Exception handling. */
    alignas(16) u8 __nx_exception_stack[ams::os::MemoryPageSize];
    u64 __nx_exception_stack_size = sizeof(__nx_exception_stack);
    void __libnx_exception_handler(ThreadExceptionDump* ctx);
}

namespace ams {

    ncm::ProgramId CurrentProgramId = { 0x010000000000bd00ul };

    namespace result {

        bool CallFatalOnResultAssertion = false;

    }

}

using namespace ams;

void __libnx_initheap(void) {
    void*  addr = nx_inner_heap;
    size_t size = nx_inner_heap_size;

    extern char* fake_heap_start;
    extern char* fake_heap_end;

    fake_heap_start = (char*)addr;
    fake_heap_end   = (char*)addr + size;
}

void __appInit(void) {
    hos::InitializeForStratosphere();

    sm::DoWithSession([&]() {
        R_ABORT_UNLESS(fsInitialize());
        R_ABORT_UNLESS(pmdmntInitialize());
        R_ABORT_UNLESS(pminfoInitialize());
    });

    R_ABORT_UNLESS(fsdevMountSdmc());

    ams::CheckApiVersion();
}

void __appExit(void) {
    btdrvExit();
    pminfoExit();
    pmdmntExit();

    fsdevUnmountAll();
    fsExit();
}

void __libnx_exception_handler(ThreadExceptionDump* ctx) {
    ams::CrashHandler(ctx);
}

ams::Result LaunchModules(void) {
    R_TRY(ams::mitm::btdrv::Launch());
    R_TRY(ams::mitm::btm::Launch());
    return ams::ResultSuccess();
}

void WaitModules(void) {
    ams::mitm::btm::WaitFinished();
    ams::mitm::btdrv::WaitFinished();
}

int main(int argc, char **argv) {
    R_ABORT_UNLESS(LaunchModules());
    WaitModules();
    
    return 0;
}
