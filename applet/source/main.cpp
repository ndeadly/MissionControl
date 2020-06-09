#include <memory>
#include <switch.h>
#include "mainapplet.hpp"

#ifdef __cplusplus
extern "C" {
#endif

//u32 __nx_applet_type = AppletType_LibraryApplet;

void userAppInit(void) {
    Result rc;

    rc = btdrvInitialize();
    if R_FAILED(rc)
        fatalThrow(rc);

    rc = btmInitialize();
    if R_FAILED(rc)
        fatalThrow(rc);

    rc = splInitialize();
    if R_FAILED(rc)
        fatalThrow(rc);

    rc = romfsInit();
    if R_FAILED(rc)
        fatalThrow(rc);

    rc = plInitialize(PlServiceType_User);
    if R_FAILED(rc)
        fatalThrow(rc);

    rc = setsysInitialize();
    if R_FAILED(rc)
        fatalThrow(rc);
}

void userAppExit(void) {   
    setsysExit();
    plExit();
    romfsExit();
    splExit();
    btmExit();
    btdrvExit();
}

#ifdef __cplusplus
}
#endif

Result btdrvextRedirectSystemEvents(bool redirect) {
    return serviceDispatchIn(btdrvGetServiceSession(), 65000, redirect);
}

void appletRun(void) {
    auto applet = std::make_unique<MainApplet>();
    applet->run();
}

int main(int argc, char* argv[]) {
    appletLockExit();
    btdrvextRedirectSystemEvents(true);
    appletRun();
    btdrvextRedirectSystemEvents(false);
    appletUnlockExit();

    return 0;
}
