#include <memory>
#include <switch.h>
#include <stratosphere.hpp>

#include "btdrv_mitm_service.hpp"
#include "bluetooth/bluetooth_events.hpp"

extern "C" {

    extern u32 __start__;

    u32 __nx_applet_type = AppletType_None;
    u32 __nx_fs_num_sessions = 1;

    #define INNER_HEAP_SIZE 0x40000
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
        //R_ABORT_UNLESS(pscmInitialize());
        R_ABORT_UNLESS(pmdmntInitialize());
        R_ABORT_UNLESS(pminfoInitialize());
        R_ABORT_UNLESS(btdrvInitialize());
    });

    R_ABORT_UNLESS(fsdevMountSdmc());

    ams::CheckApiVersion();
}

void __appExit(void) {
    btdrvExit();
    pminfoExit();
    pmdmntExit();
    //pscmExit();

    fsdevUnmountAll();
    fsExit();
}

void __libnx_exception_handler(ThreadExceptionDump* ctx) {
    ams::CrashHandler(ctx);
}

namespace {

    constexpr sm::ServiceName BtdrvMitmServiceName = sm::ServiceName::Encode("btdrv");

    struct ServerOptions {
        static constexpr size_t PointerBufferSize = 0x1000;
        static constexpr size_t MaxDomains = 0;
        static constexpr size_t MaxDomainObjects = 0;
    };

    constexpr size_t MaxServers = 1;
    constexpr size_t MaxSessions = 0x10;
    
}

int main(int argc, char **argv) {

    auto server_manager = std::make_unique<sf::hipc::ServerManager<MaxServers, ServerOptions, MaxSessions>>();
    
    R_ABORT_UNLESS(server_manager->RegisterMitmServer<ams::mitm::btdrv::BtdrvMitmService>(BtdrvMitmServiceName));

    R_ABORT_UNLESS(bluetooth::events::Initialize());

    server_manager->LoopProcess();

    return 0;
}
