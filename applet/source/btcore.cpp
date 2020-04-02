#include <cstring>

#include "application.hpp"
#include "btcore.hpp"
#include "utils.hpp"

enum BluetoothStatus {
    BluetoothStatus_Success,
    BluetoothStatus_Fail,
    BluetoothStatus_NotReady,
    BluetoothStatus_NoMemory,
    BluetoothStatus_Busy,
    BluetoothStatus_Done,
    BluetoothStatus_Unsupported,
    BluetoothStatus_ParameterInvalid,
    BluetoothStatus_Unhandled,
    BluetoothStatus_AuthenticationFailure,
    BluetoothStatus_RemoteDeviceDown,
    BluetoothStatus_AuthenticationRejected,
    BluetoothStatus_JniEnvironmentError,
    BluetoothStatus_JniThreadAttachError,
    BluetoothStatus_WakelockError
};

enum BluetoothDiscoveryState {
    BluetoothDiscoveryState_Stopped,
    BluetoothDiscoveryState_Started
};

enum BluetoothBondState {
    BluetoothBondState_None,
    BluetoothBondState_Bonding,
    BluetoothBondState_Bonded
};

namespace mc::btcore {

    namespace {

        union BluetoothEventData {
            struct {
                BluetoothName           name;
                BluetoothAddress        address;
                uint8_t                 _unk0[0x10];
                BluetoothDeviceClass    cod;
                /* + more items we don't care about */
            } deviceFound;
            
            struct {
                BluetoothDiscoveryState state;
            } discoveryState;
            
            struct {
                BluetoothAddress        address;
                BluetoothName           name;
                BluetoothDeviceClass    cod;
            } pinReply;
            
            struct {
                BluetoothAddress        address;
                BluetoothName           name;
                BluetoothDeviceClass    cod;
                BluetoothSspVariant     variant;
                uint32_t                passkey;
            } sspReply;
            
            struct {
                BluetoothAddress        address;
                BluetoothStatus         status;
                BluetoothBondState      state;
            } bondState;
        };

        const constexpr size_t BLUETOOTH_EVENT_BUFFER_SIZE = 0x400;
        const constexpr size_t BLUETOOTH_THREAD_STACK_SIZE = 0x4000;

        static Event 	btEvent;
        static Thread 	btEventThread;
        static uint8_t  btEventBuffer[BLUETOOTH_EVENT_BUFFER_SIZE];

        static bool     btDiscoveryState	 = BluetoothDiscoveryState_Stopped;
        static bool	    btBondState 		 = BluetoothBondState_None;

        static bool     discoverEnabled = false;
        static bool     pauseDiscovery  = false;
        
        void handleDeviceFoundEvent(const BluetoothEventData *eventData) {
            mc::app::log->write("btcore: device found");

            if (isController(&eventData->deviceFound.cod)) {

                /* Update timestamp and return if device already discovered */
                for (auto it = discoveredDevices.begin(); it != discoveredDevices.end(); ++it) {
                    if ( bdcmp(&(*it)->address, &eventData->deviceFound.address) ) {
                        (*it)->timestamp = armGetSystemTick();
                        return;
                    }
                }

                /* Add device to list of found devices */
                discoveredDevices.emplace_back(std::make_unique<FoundDevice>(&eventData->deviceFound.name,
                                                                             &eventData->deviceFound.address,
                                                                             &eventData->deviceFound.cod,
                                                                             armGetSystemTick()));
            }
        }

        void handleDiscoveryStateChangedEvent(const BluetoothEventData *eventData) {

            mc::app::log->write("btcore:  discovery state changed: %d", eventData->discoveryState.state);
           
            btDiscoveryState = eventData->discoveryState.state;

            if ( (btDiscoveryState == BluetoothDiscoveryState_Stopped) && discoverEnabled && !pauseDiscovery) {

                uint64_t currentTick = armGetSystemTick();

                /* Remove devices that haven't been seen in a while */
                auto it = discoveredDevices.begin();
                while (it != discoveredDevices.end()) {
                    if (armTicksToNs(currentTick - (*it)->timestamp) > 12e9L) {
                        it = discoveredDevices.erase(it);
                    }
                    else {
                        ++it;
                    }
                }

                StartDiscovery();
            }
            
        }

        void handlePinRequestEvent(const BluetoothEventData *eventData) {
            /* This doesn't actually matter because the bluetooth module ignores it */
            /* We will apply IPS patches to enable wii(U) controllers */
            BluetoothPinCode pincode = {};
            
            mc::app::log->write("btcore: sending pin reply...");
            Result rc = btdrvPinReply(&eventData->pinReply.address, false, &pincode, sizeof(BluetoothAddress));
            if R_FAILED(rc) 
                fatalThrow(rc);
        }

        void handleSspRequestEvent(const BluetoothEventData *eventData) {
            mc::app::log->write("btcore: sending ssp reply...");
            Result rc = btdrvSspReply(&eventData->sspReply.address, eventData->sspReply.variant, true, eventData->sspReply.passkey);
            if R_FAILED(rc) 
                fatalThrow(rc);
        }

        void handleBondStateChangedEvent(const BluetoothEventData *eventData) {

            mc::app::log->write("btcore: bond state changed: %d", eventData->bondState.state);
            
            btBondState = eventData->bondState.state;
            
            switch (btBondState) {
                case BluetoothBondState_Bonded:
                    {
                        Result rc;

                        mc::app::log->write("btcore: device bonded");

                        /* Retrieve bonded device details */
                        BluetoothDevice device = {};
                        rc = btdrvHidGetPairedDevice(&eventData->bondState.address, &device);
                        if R_FAILED(rc) 
                            fatalThrow(rc);
                            
                        /* Add device to the controller database */
                        rc = controllerDatabase->addDevice(&device); 

                        /* Restart device discovery if enabled */
                        pauseDiscovery = false;
                        if (discoverEnabled) {
                            StartDiscovery();
                        }                       
                    }
                    break;
                    
                case BluetoothBondState_Bonding:
                    mc::app::log->write("btcore: device bonding...");
                    break;
                case BluetoothBondState_None:
                    mc::app::log->write("btcore: bond state none");
                default:
                    break;
            }
        }

        void bluetoothEventThreadFunc(void *arg) {
            Result rc;
            BluetoothEventType eventType;
            BluetoothEventData *eventData = reinterpret_cast<BluetoothEventData *>(btEventBuffer);
            
            while (!mc::app::exitFlag) {
                if (R_SUCCEEDED(eventWait(&btEvent, 1e9))) {

                    rc = btdrvGetEventInfo(&eventType, btEventBuffer, sizeof(btEventBuffer));
                    if R_FAILED(rc) 
                        fatalThrow(rc);
                    eventClear(&btEvent);

                    switch (eventType) {
                        case BluetoothEvent_DeviceFound:
                            handleDeviceFoundEvent(eventData);
                            break;
                            
                        case BluetoothEvent_DiscoveryStateChanged:
                            handleDiscoveryStateChangedEvent(eventData);
                            break;
                            
                        case BluetoothEvent_PinRequest:
                            handlePinRequestEvent(eventData);
                            break;
                            
                        case BluetoothEvent_SspRequest:
                            handleSspRequestEvent(eventData);
                            break;
                            
                        case BluetoothEvent_BondStateChanged:
                            handleBondStateChangedEvent(eventData);
                            break;	
                            
                        default:
                            break;
                    }
                }
            }
        }

    }

    FoundDevice::FoundDevice(const BluetoothName *bdname, const BluetoothAddress *bdaddress, const BluetoothDeviceClass *bdcod, uint64_t tick) {
        memcpy(&name, bdname, sizeof(BluetoothName));
        memcpy(&address, bdaddress, sizeof(BluetoothAddress));
        memcpy(&cod, bdcod, sizeof(BluetoothDeviceClass));
        timestamp = tick;
    }

    BluetoothAddress hostAddress = {};
    std::unique_ptr<mc::controller::BluetoothDatabase> controllerDatabase = std::make_unique<mc::controller::BluetoothDatabase>();
    std::vector<std::unique_ptr<FoundDevice>> discoveredDevices;

    void Initialise(void) {
        Result rc;

        /* Fetch console bd address */
        BluetoothAdapterProperty props;
        rc = btdrvGetAdapterProperties(&props);
        if R_SUCCEEDED(rc) {
            std::memcpy(&hostAddress, &props.address, sizeof(BluetoothAddress));
        }
        
        /* Initialise bluetooth events */
        rc = btdrvInitializeBluetooth(&btEvent);
        if R_FAILED(rc) 
            fatalThrow(rc);
            
        rc = threadCreate(&btEventThread, bluetoothEventThreadFunc, nullptr, nullptr, BLUETOOTH_THREAD_STACK_SIZE, 0x2C, -2);
        if R_FAILED(rc) 
            fatalThrow(rc);
            
        rc = threadStart(&btEventThread);
        if R_FAILED(rc) 
            fatalThrow(rc);
    }

    void Cleanup(void) {
        threadWaitForExit(&btEventThread);
        threadClose(&btEventThread);

        eventClose(&btEvent);

        btdrvCleanupBluetooth();
    }

    bool IsDiscovering(void) {
        return btDiscoveryState == BluetoothDiscoveryState_Started;
    }

    bool IsPairing(void) {
        return btBondState == BluetoothBondState_Bonding;
    }

    Result StartDiscovery(void) {
        mc::app::log->write("btcore: starting discovery");
        discoverEnabled = true;
        return btdrvStartDiscovery();
    }

    Result StopDiscovery(void) {
        mc::app::log->write("btcore: stopping discovery");
        discoverEnabled = false;
        return btdrvCancelDiscovery();
    }

    Result PairDevice(const BluetoothAddress *address) {
        mc::app::log->write("btcore: pausing discovery");

        pauseDiscovery = true;
        Result rc = btdrvCancelDiscovery();
        if (R_FAILED(rc))
            fatalThrow(rc);

        mc::app::log->write("btcore: pairing device...");

        return btdrvCreateBond(address, BluetoothTransport_Auto);
    }

    Result UnpairDevice(const BluetoothAddress *address) {
        return btdrvRemoveBond(address); 
    }

    Result CancelPairing(const BluetoothAddress *address) {
        mc::app::log->write("btcore: cancelling pairing");
        return btdrvCancelBond(address);
    }

}
