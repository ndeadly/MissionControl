#include "../mc_mitm/source/ble/ble_controller_routes.hpp"
#include "../mc_mitm/source/controllers/xbox_stick.hpp"
#include "../mc_mitm/source/ble/ble_recovery.hpp"
#include <atomic>
#include <cassert>
#include <iostream>
#include <thread>

struct Controller {
    const uint32_t connection;
    explicit Controller(uint32_t id) : connection(id) {}
};
using Routes = ams::ble::ControllerRoutes<Controller, std::mutex, 4>;

static void TestRoutes() {
    Routes routes;
    // Real composite conn_ids from the original multi-controller trace.
    constexpr uint32_t ids[] = {3, 260, 517, 774};
    for (size_t i = 0; i < 4; ++i) routes.Register(i, ids[i], std::make_shared<Controller>(ids[i]));
    for (auto id : ids) assert(routes.Find(id)->connection == id);
    assert(!routes.Find(UINT32_MAX));
    assert(!routes.Find(900));

    // Reproduce the old lookup/teardown race deterministically: the input
    // thread has looked up the controller but hasn't entered its method yet.
    auto in_flight = routes.Find(260);
    std::weak_ptr<Controller> old = in_flight;
    routes.Unregister(1);
    routes.Register(1, 1028, std::make_shared<Controller>(1028));
    assert(!routes.Find(260));
    assert(in_flight->connection == 260);
    assert(!old.expired());
    in_flight.reset();
    assert(old.expired());

    std::atomic<bool> done = false;
    std::thread reader([&] {
        while (!done.load()) {
            for (uint32_t id : {260u, 1028u}) {
                auto controller = routes.Find(id);
                if (controller) assert(controller->connection == id);
            }
        }
    });
    for (unsigned i = 0; i < 100000; ++i) {
        routes.Unregister(1);
        uint32_t id = (i & 1) ? 260 : 1028;
        routes.Register(1, id, std::make_shared<Controller>(id));
    }
    done = true;
    reader.join();
}

static void TestSticks() {
    using ams::controller::xbox::ApplyRadialDeadzone;
    constexpr uint16_t center = 0x8000;
    const auto neutral = std::make_pair(center, center);
    for (int percent : {5, 15, 24, 27, 50}) {
        const int radius = 32767 * percent / 100;
        assert(ApplyRadialDeadzone(center, center, percent) == neutral);
        assert(ApplyRadialDeadzone(center + radius, center, percent) == neutral);
        assert(ApplyRadialDeadzone(center, center - radius, percent) == neutral);
        // The radial boundary is not an axis-aligned square.
        assert(ApplyRadialDeadzone(center + radius, center + radius, percent) != neutral);
        int previous = center;
        for (int x = center; x <= 65535; ++x) {
            auto [out_x, out_y] = ApplyRadialDeadzone(x, center, percent);
            assert(out_y == center);
            assert(out_x >= previous);
            previous = out_x;
            if (x > center + radius) assert(out_x >= center + 4096);
        }
        assert(previous == 65535);
        assert(ApplyRadialDeadzone(0, 0, percent) == std::make_pair(uint16_t{0}, uint16_t{0}));
        for (int d = radius + 1; d < 32767; d += 67) {
            auto positive = ApplyRadialDeadzone(center + d, center, percent);
            auto negative = ApplyRadialDeadzone(center - d, center, percent);
            assert(int(positive.first) - center == center - int(negative.first));
            auto diagonal = ApplyRadialDeadzone(center + d, center + d, percent);
            assert(diagonal.first == diagonal.second);
        }
    }
    for (int x = 0; x <= 65535; ++x) {
        assert(ApplyRadialDeadzone(x, 65535 - x, 0) == std::make_pair(uint16_t(x), uint16_t(65535 - x)));
    }
}

static void TestRecovery() {
    using Address = std::array<uint8_t, 6>;
    constexpr Address first{0x40, 0x8e, 0x2c, 0x7a, 0x3a, 0x9a};
    constexpr Address failed{0x14, 0xcb, 0x65, 0xe8, 0x2b, 0xe2};
    constexpr Address other{0x10, 0xc7, 0x35, 0xb5, 0x63, 0x19};
    ams::ble::RadioRecovery recovery;
    assert(!recovery.Begin(true, true, true, 0));
    recovery.Request();
    assert(!recovery.Begin(false, true, true, 0));
    assert(!recovery.Begin(true, false, true, 0));
    assert(!recovery.Begin(true, true, false, 0));
    assert(!recovery.Begin(true, true, true, 1));
    assert(recovery.Begin(true, true, true, 0));
    recovery.Request();
    assert(!recovery.Begin(true, true, true, 0));
    recovery.Connected(first);
    assert(!recovery.Begin(true, true, true, 0)); // successful recovery clears stale requests
    recovery.Request();
    assert(!recovery.Begin(true, true, true, 3));
    assert(recovery.Begin(true, true, true, 0));

    // Replay the supplied trace: two pads turn off, one stays connected, and
    // 14:cb refuses a new bond. Recovery must no longer wait for the last pad.
    recovery = {};
    recovery.Connected(first);
    recovery.Connected(failed);
    recovery.Connected(other);
    recovery.Request();
    recovery.Request();
    assert(!recovery.Begin(true, true, true, 1));
    recovery.PairingFailed(failed);
    assert(!recovery.Begin(false, true, true, 1));
    assert(!recovery.Begin(true, false, true, 1));
    assert(!recovery.Begin(true, true, false, 1));
    assert(recovery.Begin(true, true, true, 1));
    recovery.AwaitClients(42);
    assert(recovery.WaitingForClients());
    assert(!recovery.AcceptClients(42, 4)); // pre-reset clients, including after timeout
    assert(!recovery.AcceptClients(43, 0)); // generation changed but no usable client
    recovery.Request(); // reset-induced disconnect must not trigger a second reset
    assert(!recovery.Begin(true, true, true, 0));
    assert(recovery.AcceptClients(46, 4));
    assert(!recovery.WaitingForClients());

    // The other pad's success must not rearm resets for the failing pad.
    recovery.Connected(other);
    recovery.PairingFailed(failed);
    assert(!recovery.Begin(true, true, true, 1));
    recovery.PairingFailed(first); // cannot replace the recovery target
    recovery.Connected(first);
    assert(!recovery.Begin(true, true, true, 2));
    recovery.Connected(failed);
    assert(!recovery.Begin(true, true, true, 3));
    recovery.PairingFailed(other);
    assert(recovery.Begin(true, true, true, 2));

    // Leaving Grip/Order or sleeping during teardown postpones the reset.
    recovery.Defer();
    assert(!recovery.Begin(true, false, true, 0));
    assert(!recovery.Begin(true, true, false, 0));
    assert(recovery.Begin(true, true, true, 0));

    recovery = {};
    recovery.PairingFailed(failed);
    recovery.Connected(failed); // succeeds before the deferred reset was needed
    assert(!recovery.Begin(true, true, true, 0));
}

int main() {
    TestRecovery();
    TestRoutes();
    TestSticks();
    std::cout << "BLE recovery, routing/lifetime and Xbox deadzone regressions passed\n";
}
