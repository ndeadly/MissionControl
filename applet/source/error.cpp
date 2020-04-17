#include "error.hpp"

namespace mc::error {

    void DisplayMessage(const char *message) {
        switch(appletGetAppletType()) {
            case AppletType_Application:
            case AppletType_SystemApplication:
            {
                ErrorApplicationConfig config;
                errorApplicationCreate(&config, message, nullptr);
                errorApplicationShow(&config);
            }
            case AppletType_SystemApplet:
            case AppletType_LibraryApplet:
            case AppletType_OverlayApplet:
            {
                ErrorSystemConfig config;
                errorSystemCreate(&config, message, nullptr);
                errorSystemShow(&config);
            }
            default:
                break;
        }
    }

}
