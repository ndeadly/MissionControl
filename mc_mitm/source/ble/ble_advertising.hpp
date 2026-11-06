/*
 * Copyright (c) 2020-2026 ndeadly
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
#pragma once
#include <switch/services/btdrv_types.h>
#include <algorithm>
#include <cstring>

namespace ams::ble {

    struct AdvertisementSummary {
        char name[0x20] = {};
        bool complete_name = false;
        bool has_appearance = false;
        u16 appearance = 0;
        u8 flags = 0;
        bool has_hid_service = false;
    };

    inline AdvertisementSummary ParseAdvertisements(const BtdrvBleAdvertisement *ad_list, u8 count) {
        AdvertisementSummary out;
        for (u8 i = 0; i < std::min<u8>(count, 10); ++i) {
            const auto &ad = ad_list[i];
            const size_t len = std::min<size_t>(ad.size, sizeof(ad.data));
            switch (ad.type) {
                case BtdrvAdType_ShortenedLocalName:
                case BtdrvAdType_CompleteLocalName:
                    if (len && (!out.complete_name || ad.type == BtdrvAdType_CompleteLocalName)) {
                        const size_t n = std::min(len, sizeof(out.name) - 1);
                        std::memcpy(out.name, ad.data, n);
                        out.name[n] = '\0';
                        out.complete_name = ad.type == BtdrvAdType_CompleteLocalName;
                    }
                    break;
                case BtdrvAdType_Appearance:
                    if (len >= 2) {
                        out.has_appearance = true;
                        out.appearance = ad.data[0] | (ad.data[1] << 8);
                    }
                    break;
                case BtdrvAdType_Flags:
                    if (len) out.flags = ad.data[0];
                    break;
                case BtdrvAdType_ServiceClassUuid16ListIncomplete:
                case BtdrvAdType_ServiceClassUuid16ListComplete:
                    for (size_t j = 0; j + 1 < len; j += 2) {
                        if (ad.data[j] == 0x12 && ad.data[j + 1] == 0x18) out.has_hid_service = true;
                    }
                    break;
                default:
                    break;
            }
        }
        return out;
    }

    inline bool IsBleAdvertisement(const AdvertisementSummary &summary, u8 device_type) {
        // Keep Classic-only inquiry results out of GATT. LE-only flags are
        // stronger evidence than an absent/zero transport field in the wrapper.
        return device_type != 0 || (summary.flags & 0x04);
    }

    inline bool IsXboxCandidate(const AdvertisementSummary &summary) {
        if (summary.name[0]) return std::strncmp(summary.name, "Xbox", 4) == 0;

        // Xbox puts its name in a separate scan response. Preserve the probe's
        // appearance fallback when that response is absent. This identifies a
        // candidate, not a vendor: appearance alone is shared by other gamepads.
        return summary.has_appearance && summary.appearance == 0x03c4;
    }

}
