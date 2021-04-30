/*
 * Copyright (c) 2020-2021 ndeadly
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

    R_ABORT_UNLESS(smInitialize());
    R_ABORT_UNLESS(fsInitialize());
    R_ABORT_UNLESS(pmdmntInitialize());
    R_ABORT_UNLESS(pminfoInitialize());
}

void __appExit(void) {
    btdrvExit();
    pminfoExit();
    pmdmntExit();
    fsExit();
    smExit();
}

void __libnx_exception_handler(ThreadExceptionDump* ctx) {
    ams::CrashHandler(ctx);
}

int main(int argc, char **argv) {
    // Parse global module settings ini from sd card
    ams::mitm::ParseIniConfig();

    // Start initialisation thread
    ams::mitm::StartInitialize();

    // Launch mitm modules
    ams::mitm::LaunchModules();

    // Wait for mitm modules to terminate
    ams::mitm::WaitModules();

    return 0;
}
