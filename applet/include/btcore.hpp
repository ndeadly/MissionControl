#pragma once

#include <memory>
#include <vector>

#include <switch.h>
#include "bluetoothdatabase.hpp"


namespace mc::btcore {

    struct FoundDevice {
        BluetoothName           name;
        BluetoothAddress        address;
        BluetoothDeviceClass    cod;
        uint64_t                timestamp;

        FoundDevice(const BluetoothName *name, const BluetoothAddress *address, const BluetoothDeviceClass *cod, uint64_t tick);
    };

    void Initialise(void);
    void Cleanup(void);

    bool   IsDiscovering(void);
    bool   IsPairing(void);

    Result StartDiscovery(void);
    Result StopDiscovery(void);
    Result PairDevice(const BluetoothAddress *address);
    Result UnpairDevice(const BluetoothAddress *address);
    Result CancelPairing(const BluetoothAddress *address);

    extern BluetoothAddress hostAddress;
    extern std::unique_ptr<mc::controller::BluetoothDatabase> controllerDatabase;
    extern std::vector<std::unique_ptr<FoundDevice>> discoveredDevices;
    
}
