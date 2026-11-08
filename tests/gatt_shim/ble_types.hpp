#pragma once
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <deque>
using u8 = uint8_t; using u16 = uint16_t; using u32 = uint32_t; using u64 = uint64_t;
struct Result { int value; int GetValue() const { return value; } };
#define R_SUCCEEDED(r) ((r).value == 0)
#define R_FAILED(r) (!R_SUCCEEDED(r))
#define R_THROW(r) return (r)
#define R_RETURN(r) return (r)
#define R_SUCCEED() return Result{0}
#define R_TRY(r) do { auto rc_ = (r); if (R_FAILED(rc_)) return rc_; } while (0)
#define R_UNLESS(p,r) do { if (!(p)) return (r); } while (0)
inline Result ResultSuccess() { return {0}; }
namespace svc {
inline Result ResultInvalidArgument() { return {1}; }
inline Result ResultTimedOut() { return {2}; }
inline Result ResultNotFound() { return {3}; }
inline Result ResultInvalidState() { return {8}; }
}
struct TimeSpan {
    u64 milliseconds;
    static TimeSpan FromSeconds(int seconds) { return {u64(seconds) * 1000}; }
    static TimeSpan FromMilliSeconds(int milliseconds) { return {u64(milliseconds)}; }
};
struct BtdrvGattAttributeUuid { u16 value; };
struct BtdrvGattId { BtdrvGattAttributeUuid uuid; u8 instance_id = 0; };
struct BtdrvGattAttribute {
    u8 type = 0; u16 handle = 0; u16 group_end_handle = 0; u8 property = 0;
    bool is_primary = true; BtdrvGattId id = {};
};
constexpr int BtdrvGattAttributeType_Service = 1;
constexpr int BtdrvGattAttributeType_Characteristic = 2;
constexpr int BtdrvGattAttributeType_Descriptor = 3;
constexpr int BtdrvGattCharacteristicProperty_Notify = 1;
constexpr int BtdrvGattCharacteristicProperty_Write = 2;
constexpr int BtdrvGattCharacteristicProperty_WriteNoResponse = 4;
constexpr int BtdrvGattAuthReqType_NoMitm = 1;
constexpr int BtdrvGattOperationType_CharacteristicRead = 0;
constexpr int BtdrvGattOperationType_DescriptorWrite = 3;
constexpr int BtdrvBleEventType_ClientCacheSave = 1;
constexpr int BtdrvBleEventType_ClientConfigureMtu = 2;
constexpr int BtdrvBleEventType_ClientNotify = 3;
namespace ams::bluetooth { struct Address { u8 data[6]; }; }
namespace ams::ble {
constexpr u32 MaxBleControllers = 4, InvalidSlot = UINT32_MAX, InvalidConnId = UINT32_MAX;
constexpr u16 GattServiceHid = 0x1812, GattCharReport = 0x2a4d, GattCharHidInformation = 0x2a4a, GattCharReportMap = 0x2a4b, GattDescClientCharConfig = 0x2902;
inline bool IsUuid16(BtdrvGattAttributeUuid uuid, u16 value) { return uuid.value == value; }
inline bool AddressEqual(const bluetooth::Address &a, const bluetooth::Address &b) { return std::memcmp(&a, &b, sizeof(a)) == 0; }
struct EventRecord {
    int type = 0;
    struct {
        struct { u32 result = 0, conn_id = 0, count = 0; BtdrvGattAttribute attr_list[10]; } client_cache_save;
        struct { u32 result = 0, conn_id = 0; u16 mtu = 0; } client_configure_mtu;
        struct { u32 result = 0, conn_id = 0; u8 op_type = 0; BtdrvGattAttributeUuid char_uuid = {}; } client_notify;
    } info;
};
}
Result btdrvGetGattAttribute(ams::bluetooth::Address, u32);
Result btdrvConfigureAttMtu(u32, u16);
Result btdrvReadGattCharacteristic(u32, bool, const BtdrvGattId *, const BtdrvGattId *, int);
Result btdrvRegisterGattNotification(u32, bool, const BtdrvGattId *, const BtdrvGattId *);
Result btdrvUnregisterGattNotification(u32, bool, const BtdrvGattId *, const BtdrvGattId *);
Result btdrvWriteGattDescriptor(u32, bool, const BtdrvGattId *, const BtdrvGattId *, const BtdrvGattId *, const u8 *, size_t, int);
