#include <cstring>

#include "application.hpp"
#include "bluetooth/core.hpp"
#include "utils.hpp"

namespace mc::bluetooth::core {

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
            
            union {
                struct {
                    BluetoothAddress        address;
                    BluetoothStatus         status;
                    BluetoothBondState      state;
                };
                struct {
                    BluetoothStatus         status;
                    BluetoothAddress        address;
                    BluetoothBondState      state;
                } v2;
            } bondState;
        };

        const constexpr size_t BLUETOOTH_EVENT_BUFFER_SIZE = 0x400;
        const constexpr size_t BLUETOOTH_THREAD_STACK_SIZE = 0x4000;

        static Event 	btmDiscoveryEvent;
        static Thread 	btmDiscoveryEventThread;

        static Event 	btEvent;
        static Thread 	btEventThread;
        static uint8_t  btEventBuffer[BLUETOOTH_EVENT_BUFFER_SIZE];

        static BluetoothDiscoveryState btDiscoveryState	 = BluetoothDiscoveryState_Stopped;
        static Mutex                   btDiscoveryStateMutex;

        static BluetoothBondState	   btBondState 		 = BluetoothBondState_None;
        static Mutex                   btBondStateMutex;

        static bool     discoverEnabled = false;
        static bool     pauseDiscovery  = false;

        Result registerDevice(const BluetoothDevice *device) {
            BtmDeviceInfo dst = {0};
            memset(&dst, 0, sizeof(BtmDeviceInfo));
            memcpy(&dst.address, &device->address, sizeof(BluetoothAddress));
            memcpy(&dst.device_class, &device->device_class, sizeof(BluetoothDeviceClass));
            //strncpy(&dst.name, device->name, sizeof(BluetoothRemoteName)-1);
            memcpy(&dst.name, device->name, sizeof(BluetoothRemoteName)-1);
            memcpy(&dst.link_key, &device->link_key, sizeof(BluetoothLinkKey));
            dst.vid = device->vid;
            dst.pid = device->pid;

            return btmAddDeviceInfo(&dst);
        }
        
        void handleDeviceFoundEvent(const BluetoothEventData *eventData) {
            //mc::app::log->write("bluetooth::core: device found");

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

            //mc::app::log->write("bluetooth::core:  discovery state changed: %d", eventData->discoveryState.state);
           
            //mutexLock(&btDiscoveryStateMutex);
            btDiscoveryState = eventData->discoveryState.state;
            //mutexUnlock(&btDiscoveryStateMutex);

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
            
            //mc::app::log->write("bluetooth::core: sending pin reply...");
            Result rc = btdrvRespondToPinRequest(&eventData->pinReply.address, false, &pincode, sizeof(BluetoothAddress));
            if R_FAILED(rc) 
                fatalThrow(rc);
        }

        void handleSspRequestEvent(const BluetoothEventData *eventData) {
            //mc::app::log->write("bluetooth::core: sending ssp reply...");
            Result rc = btdrvRespondToSspRequest(&eventData->sspReply.address, eventData->sspReply.variant, true, eventData->sspReply.passkey);
            if R_FAILED(rc) 
                fatalThrow(rc);
        }

        void handleBondStateChangedEvent(const BluetoothEventData *eventData) {

            //mc::app::log->write("bluetooth::core: bond state changed: %d", eventData->bondState.state);
            
            //mutexLock(&btBondStateMutex);
            btBondState = hosversionBefore(9, 0, 0) ? eventData->bondState.state : eventData->bondState.v2.state;
            //mutexUnlock(&btBondStateMutex);
            
            switch (btBondState) {
                case BluetoothBondState_Bonded:
                    {
                        //mc::app::log->write("bluetooth::core: device bonded");
                        const BluetoothAddress *address = hosversionBefore(9, 0, 0) ? &eventData->bondState.address : &eventData->bondState.v2.address;
                        
                        /* Retrieve bonded device details */
                        BluetoothDevice device = {};
                        Result rc = btdrvGetPairedDeviceInfo(address, &device);
                        if R_FAILED(rc) 
                            fatalThrow(rc);
                            
                        /* Add device to the controller database */
                        //rc = controllerDatabase->addDevice(&device);

                        /* Disconnect controller */
                        rc = btdrvCloseHidConnection(address);
                        if R_FAILED(rc) 
                            fatalThrow(rc);

                        svcSleepThread(2e8);
                        
                        rc = btdrvRemoveBond(address);
                        if R_FAILED(rc) 
                            fatalThrow(rc);

                        svcSleepThread(2e8);

                        /* Register device with btm module */
                        rc = registerDevice(&device);
                        if R_FAILED(rc) 
                            fatalThrow(rc);

                        svcSleepThread(1e9);

                        //rc = btdrvOpenHidConnection(address);
                        rc = btmdbgHidConnect(address);
                        if R_FAILED(rc) 
                            fatalThrow(rc);
                        
                    }
                    break;
                    
                case BluetoothBondState_Bonding:
                    //mc::app::log->write("bluetooth::core: device bonding...");
                    {

                    }
                    break;
                case BluetoothBondState_None:
                    //mc::app::log->write("bluetooth::core: bond state none");
                    {
                    /* Restart device discovery if enabled */
                    pauseDiscovery = false;
                    if (discoverEnabled) {
                        StartDiscovery();
                    }     
    
                    }
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

                    /* Crashes console on 10.0.0 */
                    
                    /*
                    rc = btdrvGetEventInfo(&eventType, btEventBuffer, sizeof(btEventBuffer));
                    if R_FAILED(rc) 
                        fatalThrow(rc);
                    */
                    
                    //eventClear(&btEvent);

                    /*
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
                    */
                    
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
    //std::unique_ptr<mc::controller::BluetoothDatabase> controllerDatabase = std::make_unique<mc::controller::BluetoothDatabase>();
    std::vector<std::unique_ptr<FoundDevice>> discoveredDevices;

    void Initialise(void) {
        Result rc;

        //mutexInit(&btDiscoveryStateMutex);
        //mutexInit(&btBondStateMutex);

        /* Fetch console bd address */
        BluetoothAdapterProperty props;
        rc = btdrvGetAdapterProperties(&props);
        if (R_SUCCEEDED(rc)) {
            std::memcpy(&hostAddress, &props.address, sizeof(BluetoothAddress));
        }
        
        /* Initialise bluetooth events */
        /* Crashes console on 10.0.0 */
        rc = btdrvInitializeBluetooth(&btEvent);
        if (R_FAILED(rc) )
            fatalThrow(rc);
        
        u8 flags;
        rc = btmdbgAcquireDiscoveryEvent(&btmDiscoveryEvent, &flags);
        if (R_FAILED(rc))
            fatalThrow(rc);

        rc = threadCreate(&btEventThread, bluetoothEventThreadFunc, nullptr, nullptr, BLUETOOTH_THREAD_STACK_SIZE, 0x2C, -2);
        if (R_FAILED(rc))
            fatalThrow(rc);
        
        /*
        rc = threadStart(&btEventThread);
        if (R_FAILED(rc))
            fatalThrow(rc);
        */
    }

    void Finalise(void) {
        threadWaitForExit(&btEventThread);
        threadClose(&btEventThread);

        //threadWaitForExit(&btmDiscoveryEventThread);
        //threadClose(&btmDiscoveryEventThread);

        eventClose(&btEvent);
        eventClose(&btmDiscoveryEvent);

        /* Crashes console on 10.0.0 */
        btdrvFinalizeBluetooth();
    }

    bool IsDiscovering(void) {
        //mutexLock(&btDiscoveryStateMutex);
        bool discovering = btDiscoveryState == BluetoothDiscoveryState_Started;
        //mutexUnlock(&btDiscoveryStateMutex);
        return discovering;
    }

    bool IsPairing(void) {
        //mutexLock(&btBondStateMutex);
        bool pairing = btBondState == BluetoothBondState_Bonding;
        //mutexUnlock(&btBondStateMutex);
        return pairing;
    }

    Result StartDiscovery(void) {
        //mc::app::log->write("bluetooth::core: starting discovery");
        discoverEnabled = true;
        return btdrvStartInquiry();
    }

    Result StopDiscovery(void) {
        //mc::app::log->write("bluetooth::core: stopping discovery");
        discoverEnabled = false;
        return btdrvStopInquiry();
    }

    Result PairDevice(const BluetoothAddress *address) {
        //mc::app::log->write("bluetooth::core: pausing discovery");
        //mc::app::log->write("bluetooth::core: pairing device...");
        pauseDiscovery = true;
        Result rc = btdrvStopInquiry();
        if (R_FAILED(rc))
            fatalThrow(rc);
            
        return btdrvCreateBond(address, BluetoothTransport_Auto);
    }

    Result UnpairDevice(const BluetoothAddress *address) {
        return btdrvRemoveBond(address); 
    }

    Result CancelPairing(const BluetoothAddress *address) {
        //mc::app::log->write("bluetooth::core: cancelling pairing");
        return btdrvCancelBond(address);
    }

}
