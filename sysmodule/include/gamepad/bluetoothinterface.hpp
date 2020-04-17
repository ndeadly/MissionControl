#pragma once

#include <switch.h>
#include "gamepad/hidinterface.hpp"

namespace mc::controller {

	class BluetoothInterface : public HidInterface {

		public:
			BluetoothInterface() : HidInterface(HidInterfaceType_Bluetooth) {};

			Result connect(void);
			Result disconnect(void);
			Result wake(void);
			Result sendData(const uint8_t *buffer, uint16_t length);

			const BluetoothAddress& address(void) const;
			void setAddress(const BluetoothAddress *address);
					
		private:
			BluetoothAddress m_address;
			
	};

}
