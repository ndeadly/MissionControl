#pragma once
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <deque>
using u8 = uint8_t; using u16 = uint16_t; using u32 = uint32_t; using u64 = uint64_t;
struct Result { unsigned value; unsigned GetValue() const { return value; } };
#define R_FAILED(r) ((r).value != 0)
#define R_SUCCEEDED(r) (!R_FAILED(r))
#define R_THROW(r) return (r)
#define R_RETURN(r) return (r)
#define R_SUCCEED() return Result{0}
#define R_TRY(r) do { auto rc_ = (r); if (R_FAILED(rc_)) return rc_; } while (0)
#define R_UNLESS(p,r) do { if (!(p)) return (r); } while (0)
namespace svc { inline Result ResultTimedOut() { return {2}; } }
struct TimeSpan {
    u64 milliseconds;
    static TimeSpan FromSeconds(int seconds) { return {u64(seconds) * 1000}; }
    static TimeSpan FromMilliSeconds(int ms) { return {u64(ms)}; }
};
namespace ams::bluetooth { struct Address { u8 address[6]; }; }
constexpr int BtdrvBleEventType_ClientConnection = 1;
namespace ams::ble {
constexpr u32 InvalidConnId = UINT32_MAX;
constexpr u32 MaxBleControllers = 4;
constexpr u8 InvalidClientIf = UINT8_MAX;
constexpr u32 BleConnectionStatus_Connected = 0;
inline bool AddressEqual(const bluetooth::Address &a, const bluetooth::Address &b) { return std::memcmp(&a, &b, sizeof(a)) == 0; }
struct EventRecord {
    int type = 0;
    struct {
        struct {
            u32 result = 0, status = 0;
            u8 client_if = 3;
            u32 conn_id = 259;
            bluetooth::Address address = {};
            u16 reason = 0;
        } client_connection;
    } info;
};
}
Result btdrvRemoveBond(ams::bluetooth::Address);
Result btdrvCancelBond(ams::bluetooth::Address);
Result btdrvConnectGattServer(u8, ams::bluetooth::Address, bool, u64);
Result btdrvCancelConnectGattServer(u8, ams::bluetooth::Address, bool);
Result btdrvCreateBond(ams::bluetooth::Address, u32);
