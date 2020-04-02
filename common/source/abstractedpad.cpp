#include <cstring>
#include "abstractedpad.hpp"

namespace mc::controller {

	AbstractedPadVirtualController::AbstractedPadVirtualController() {
		std::memset(&m_state, 0, sizeof(HiddbgAbstractedPadState));
		m_state.type = BIT(0);  //maybe equivalent to HidDeviceTypeBits_FullKey
		m_state.npadInterfaceType = NpadInterfaceType_Bluetooth;
		m_state.flags = 0xff;    //Don't know if needs to be this value
		m_state.state.batteryCharge = 4;
		m_state.singleColorBody = RGBA8_MAXALPHA(45, 45, 45);
		m_state.singleColorButtons = RGBA8_MAXALPHA(230, 230, 230);
		
		/* Todo: keep track of assigned ids and set this properly */
		m_uniqueId = 0;
	}

	AbstractedPadVirtualController::~AbstractedPadVirtualController() {
		this->disconnect();
	}

	Result AbstractedPadVirtualController::connect(void) {
		return hiddbgSetAutoPilotVirtualPadState(m_uniqueId, &m_state);
	}

	Result AbstractedPadVirtualController::disconnect(void) {
		return hiddbgUnsetAutoPilotVirtualPadState(m_uniqueId);
	}

	Result AbstractedPadVirtualController::setState(const SwitchProGamepadState* state) {
		m_state.state.buttons = 0;

		m_state.state.buttons |= state->A             ? KEY_A : 0;
		m_state.state.buttons |= state->B             ? KEY_B : 0;
		m_state.state.buttons |= state->X             ? KEY_X : 0;
		m_state.state.buttons |= state->Y             ? KEY_Y : 0;

		m_state.state.buttons |= state->dpad_down     ? KEY_DDOWN : 0;
		m_state.state.buttons |= state->dpad_up       ? KEY_DUP : 0;
		m_state.state.buttons |= state->dpad_right    ? KEY_DRIGHT : 0;
		m_state.state.buttons |= state->dpad_left     ? KEY_DLEFT : 0;

		m_state.state.buttons |= state->L             ? KEY_L : 0;
		m_state.state.buttons |= state->ZL            ? KEY_ZL : 0;
		m_state.state.buttons |= state->lstick_press  ? KEY_LSTICK : 0;

		m_state.state.buttons |= state->R             ? KEY_R : 0;
		m_state.state.buttons |= state->ZR            ? KEY_ZR : 0;
		m_state.state.buttons |= state->rstick_press  ? KEY_RSTICK : 0;

		m_state.state.buttons |= state->minus          ? KEY_MINUS : 0;
		m_state.state.buttons |= state->plus           ? KEY_PLUS : 0;
		m_state.state.buttons |= state->capture        ? KEY_CAPTURE : 0;
		m_state.state.buttons |= state->home           ? KEY_HOME : 0;

		m_state.state.joysticks[JOYSTICK_LEFT].dx = state->left_stick.dx;
		m_state.state.joysticks[JOYSTICK_LEFT].dy = state->left_stick.dy;
		
		m_state.state.joysticks[JOYSTICK_RIGHT].dx = state->right_stick.dx;
		m_state.state.joysticks[JOYSTICK_RIGHT].dy = state->right_stick.dy;

		return hiddbgSetAutoPilotVirtualPadState(m_uniqueId, &m_state);
	}

}
