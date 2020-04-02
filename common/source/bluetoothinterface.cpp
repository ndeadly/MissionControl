#include <cstring>
#include "bluetoothinterface.hpp"

namespace mc::controller {

	Result BluetoothInterface::connect(void) {
		return btdrvHidConnect(&m_address);
	}

	Result BluetoothInterface::disconnect(void) {
		return btdrvHidDisconnect(&m_address);
	}

	Result BluetoothInterface::wake(void) {
		return btdrvHidWakeController(&m_address);
	}

	Result BluetoothInterface::sendData(const uint8_t *buffer, uint16_t length) {
		return btdrvHidSendData2(&m_address, buffer, length);
	}

	const BluetoothAddress& BluetoothInterface::address(void) const {
		return m_address;
	}

	void BluetoothInterface::setAddress(const BluetoothAddress *address) {
		std::memcpy(&m_address, address, sizeof(BluetoothAddress));
	}

}
