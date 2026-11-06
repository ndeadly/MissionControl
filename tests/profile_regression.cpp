// Compile the production profiler against Horizon-like file size semantics.
#include "ble_profile.cpp"
#include "ble_log.hpp"
#include <iostream>

using namespace ams;
namespace profile = ams::ble::profile;

static void Report(s64 arrival_us, s64 processing_us) {
    const auto writes_before = fs::writes;
    const auto opens_before = fs::opens;
    os::now_us = arrival_us;
    const auto start = profile::RecordReportStart();
    os::now_us += processing_us;
    profile::RecordReportEnd(start);
    assert(fs::writes == writes_before && fs::opens == opens_before); // input thread does no SD I/O
}

int main() {
    profile::Initialize();
    assert(profile::IsEnabled() && fs::contents.empty());
    profile::FlushNow();
    assert(fs::writes == 0); // no samples
    profile::RecordConnectionUpdate(12, 0, 500);
    for (int i = 0; i < 499; ++i) Report(1000 + i * 15000, 10);
    profile::FlushIfDue();
    assert(fs::writes == 0);
    Report(1000 + 499 * 15000, 10);
    profile::FlushIfDue();
    assert(fs::writes == 1 && !fs::contents.empty());
    assert(fs::opens == fs::closes);
    assert(fs::contents.find("Total Reports Processed : 500\n") != std::string::npos);
    assert(fs::contents.find("Min Interval : 15.00 ms") != std::string::npos);
    assert(fs::contents.find("Avg Interval : 15.00 ms") != std::string::npos);
    assert(fs::contents.find("Avg Processing Time : 10 us") != std::string::npos);
    assert(fs::contents.find("combined stream") != std::string::npos);
    assert(fs::contents.find('\0') == std::string::npos);
    profile::FlushIfDue();
    assert(fs::writes == 1); // snapshot consumed once

    const auto long_size = fs::contents.size();
    profile::Reset();
    Report(1000, 1);
    profile::FlushNow();
    assert(fs::contents.find("Total Reports Processed : 1\n") != std::string::npos);
    assert(fs::contents.find("Avg Interval : 0.00 ms") != std::string::npos);
    assert(fs::contents.size() < long_size);
    assert(fs::contents.ends_with("=====================================================\n\n")); // no stale suffix
    assert(fs::contents.find('\0') == std::string::npos);

    // Reconnecting after a long idle/sleep gap starts a fresh cadence window,
    // while the boot-wide report counter remains intact.
    profile::BeginSession();
    Report(180000000, 5);
    Report(180015000, 5);
    profile::FlushNow();
    assert(fs::contents.find("Total Reports Processed : 3\n") != std::string::npos);
    assert(fs::contents.find("Max Interval : 15.00 ms") != std::string::npos);

    // Report each filesystem failure and close every successful open.
    fs::fail_resize = true;
    auto writes_before = fs::writes;
    profile::FlushNow();
    assert(fs::writes == writes_before);
    assert(ble::log::messages.back().find("resize failed") != std::string::npos);
    fs::fail_resize = false;
    fs::fail_write = true;
    profile::FlushNow();
    assert(ble::log::messages.back().find("write failed") != std::string::npos);
    fs::fail_write = false;
    fs::fail_open = true;
    profile::FlushNow();
    assert(ble::log::messages.back().find("open failed") != std::string::npos);
    fs::fail_open = false;
    assert(fs::opens == fs::closes);

    fs::fail_create = true;
    profile::Initialize();
    assert(!profile::IsEnabled());
    assert(ble::log::messages.back().find("create failed") != std::string::npos);
    writes_before = fs::writes;
    Report(2000, 10);
    profile::FlushNow();
    assert(fs::writes == writes_before);
    std::cout << "BLE production profiler regressions passed\n";
}
