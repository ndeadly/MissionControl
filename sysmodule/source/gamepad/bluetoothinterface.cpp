#include <cstring>
#include "gamepad/bluetoothinterface.hpp"

namespace mc::controller {

	Result BluetoothInterface::connect(void) {
		return btdrvOpenHidConnection(&m_address);
	}

	Result BluetoothInterface::disconnect(void) {
		return btdrvCloseHidConnection(&m_address);
	}

	Result BluetoothInterface::wake(void) {
		return btdrvTriggerConnection(&m_address, 0);
	}

	Result BluetoothInterface::sendData(const uint8_t *buffer, uint16_t length) {
		return btdrvWriteHidData2(&m_address, buffer, length);
	}

	const BluetoothAddress& BluetoothInterface::address(void) const {
		return m_address;
	}

	void BluetoothInterface::setAddress(const BluetoothAddress *address) {
		std::memcpy(&m_address, address, sizeof(BluetoothAddress));
	}

}
