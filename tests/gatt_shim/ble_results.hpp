#pragma once
namespace ams::ble {
inline Result ResultNoConnection() { return {4}; }
struct ResultInsufficientAuthentication : Result {
    ResultInsufficientAuthentication() : Result{5} {}
    static bool Includes(Result r) { return r.value == 5; }
};
}
