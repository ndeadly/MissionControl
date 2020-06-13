#include <cstring>
#include "hdlsvirtualcontroller.hpp"

#include "../btdrv_mitm_logging.hpp"

namespace controller {

    HdlsVirtualController::HdlsVirtualController() {
        BTDRV_LOG_FMT("HdlsVirtualController() called");
        m_handle = INVALID_HANDLE;

        std::memset(&m_device, 0, sizeof(HiddbgHdlsDeviceInfo));
        m_device.deviceType         = HidDeviceType_FullKey3;
        m_device.npadInterfaceType  = NpadInterfaceType_Bluetooth;
        // Set official pro controller colours
        m_device.singleColorBody      = RGBA8_MAXALPHA(45, 45, 45);
        m_device.singleColorButtons   = RGBA8_MAXALPHA(230, 230, 230);
        m_device.colorLeftGrip        = RGBA8_MAXALPHA(70, 70, 70);
        m_device.colorRightGrip       = RGBA8_MAXALPHA(70, 70, 70);

        std::memset(&m_state, 0, sizeof(HiddbgHdlsState));
        m_state.batteryCharge = 4;	

        this->connect();
    }

    HdlsVirtualController::~HdlsVirtualController() {
        BTDRV_LOG_FMT("~HdlsVirtualController() called");
        this->disconnect();
    }

    Result HdlsVirtualController::connect(void) {
        BTDRV_LOG_FMT("Connecting Hdls virtual device");
        return hiddbgAttachHdlsVirtualDevice(&m_handle, &m_device);
    }

    Result HdlsVirtualController::disconnect(void) {
        BTDRV_LOG_FMT("Disconnecting Hdls virtual device");
        return hiddbgDetachHdlsVirtualDevice(m_handle);
    }

    Result HdlsVirtualController::setState(const SwitchProGamepadState* state) {
        if (m_handle != INVALID_HANDLE) {
            m_state.buttons = 0;

            m_state.buttons |= state->A             ? KEY_A : 0;
            m_state.buttons |= state->B             ? KEY_B : 0;
            m_state.buttons |= state->X             ? KEY_X : 0;
            m_state.buttons |= state->Y             ? KEY_Y : 0;

            m_state.buttons |= state->dpad_down     ? KEY_DDOWN : 0;
            m_state.buttons |= state->dpad_up       ? KEY_DUP : 0;
            m_state.buttons |= state->dpad_right    ? KEY_DRIGHT : 0;
            m_state.buttons |= state->dpad_left     ? KEY_DLEFT : 0;

            m_state.buttons |= state->L             ? KEY_L : 0;
            m_state.buttons |= state->ZL            ? KEY_ZL : 0;
            m_state.buttons |= state->lstick_press  ? KEY_LSTICK : 0;

            m_state.buttons |= state->R             ? KEY_R : 0;
            m_state.buttons |= state->ZR            ? KEY_ZR : 0;
            m_state.buttons |= state->rstick_press  ? KEY_RSTICK : 0;

            m_state.buttons |= state->minus          ? KEY_MINUS : 0;
            m_state.buttons |= state->plus           ? KEY_PLUS : 0;
            m_state.buttons |= state->capture        ? KEY_CAPTURE : 0;
            m_state.buttons |= state->home           ? KEY_HOME : 0;

            m_state.joysticks[JOYSTICK_LEFT].dx = state->left_stick.dx;
            m_state.joysticks[JOYSTICK_LEFT].dy = state->left_stick.dy;
            
            m_state.joysticks[JOYSTICK_RIGHT].dx = state->right_stick.dx;
            m_state.joysticks[JOYSTICK_RIGHT].dy = state->right_stick.dy;

            return hiddbgSetHdlsState(m_handle, &m_state);
        }

        return -1;
    }

}