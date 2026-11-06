#include "../mc_mitm/source/ble/ble_advertising.hpp"
#include <cassert>
#include <iostream>

using namespace ams::ble;

static BtdrvBleAdvertisement Name(const char *name, bool complete = true) {
    BtdrvBleAdvertisement ad = {};
    ad.type = complete ? BtdrvAdType_CompleteLocalName : BtdrvAdType_ShortenedLocalName;
    ad.size = std::min(std::strlen(name), sizeof(ad.data));
    std::memcpy(ad.data, name, ad.size);
    return ad;
}

int main() {
    // Xbox separates this advertisement from the name-bearing scan response.
    // Fixture follows the captured packets in esp32beans/picow_bt's README.
    BtdrvBleAdvertisement ads[10] = {
        {1, BtdrvAdType_Flags, {0x06}},
        {1, BtdrvAdType_TxPowerLevel, {20}},
        {2, BtdrvAdType_Appearance, {0xc4, 0x03}},
        {2, BtdrvAdType_ServiceClassUuid16ListComplete, {0x12, 0x18}},
    };
    auto summary = ParseAdvertisements(ads, 4);
    assert(summary.name[0] == 0);
    assert(summary.has_hid_service);
    // The previous name-only matcher rejected this valid candidate.
    assert(std::strncmp(summary.name, "Xbox", 4) != 0);
    assert(IsXboxCandidate(summary));
    assert(IsBleAdvertisement(summary, 1));
    assert(IsBleAdvertisement(summary, 0)); // explicit LE-only flags

    // A separate scan response also works without appearance/flags.
    ads[0] = Name("Xbox Wireless Controller");
    summary = ParseAdvertisements(ads, 1);
    assert(IsXboxCandidate(summary));
    assert(IsBleAdvertisement(summary, 1));
    assert(IsBleAdvertisement(summary, 2));
    assert(!IsBleAdvertisement(summary, 0)); // Classic Xbox stays on Classic

    // Other named controllers must not be decoded with the Xbox layout.
    ads[0] = Name("Pro Controller");
    summary = ParseAdvertisements(ads, 4);
    assert(!IsXboxCandidate(summary));
    ads[0] = Name("Xbox Wireless Controller");
    ads[1] = Name("Xb", false);
    summary = ParseAdvertisements(ads, 4);
    assert(std::strcmp(summary.name, "Xbox Wireless Controller") == 0);
    std::swap(ads[0], ads[1]);
    assert(IsXboxCandidate(ParseAdvertisements(ads, 4)));

    // Missing/truncated/malformed fields must stay bounded, and cannot turn a
    // mouse or an empty advertisement into a gamepad.
    assert(!IsXboxCandidate(ParseAdvertisements(ads, 0)));
    ads[0] = {2, BtdrvAdType_Appearance, {0xc2, 0x03}};
    assert(!IsXboxCandidate(ParseAdvertisements(ads, 1)));
    ads[0] = {1, BtdrvAdType_Appearance, {0xc4, 0x03}};
    assert(!IsXboxCandidate(ParseAdvertisements(ads, 1)));
    ads[0] = Name("Xbox Wireless Controller");
    ads[0].size = 255;
    std::memset(ads[0].data, 'x', sizeof(ads[0].data));
    for (int count = 0; count <= 255; ++count) {
        summary = ParseAdvertisements(ads, count);
        assert(std::memchr(summary.name, '\0', sizeof(summary.name)) != nullptr);
    }
    for (int size = 0; size <= 255; ++size) {
        for (u8 type : {u8(BtdrvAdType_ServiceClassUuid16ListComplete), u8(BtdrvAdType_Appearance), u8(BtdrvAdType_Flags)}) {
            ads[0].size = size;
            ads[0].type = type;
            (void)ParseAdvertisements(ads, 255);
        }
    }
    std::cout << "BLE advertisement selection regressions passed\n";
}
