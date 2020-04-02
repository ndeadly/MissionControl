#include "hidgamepad.hpp"
#include "bluetoothinterface.hpp"

namespace mc::controller {

	namespace {

		const constexpr int default_deadzone_percent = 0.05;

	}

	HidGamepad::HidGamepad(HidInterfaceType iface) {
		/* Assign HIDinterface */
		switch (iface) {
			case HidInterfaceType_Bluetooth:
				m_btInterface = std::make_unique<BluetoothInterface>();
				break;
			case HidInterfaceType_Usb:	// Not yet implemented
				m_btInterface = nullptr;
				break;
			default:
				fatalThrow(-1);
		}
		
		/* Assign virtual controller */
		if (hosversionAtLeast(7, 0, 0)) {
			m_virtual = std::make_unique<HdlsVirtualController>();
		} else if (hosversionAtLeast(5, 0, 0)) {
			m_virtual = std::make_unique<AbstractedPadVirtualController>();
		} else {
			fatalThrow(-1);
		}
		
		this->setInnerDeadzone(default_deadzone_percent);
		this->setOuterDeadzone(default_deadzone_percent);

		m_virtual->connect();
	}

	void HidGamepad::setInnerDeadzone(float percentage) {
		m_innerDeadzone = JOYSTICK_MAX * percentage;
	}

	void HidGamepad::setOuterDeadzone(float percentage) {
		m_outerDeadzone = JOYSTICK_MAX * (1.0 - percentage);
	}

}
