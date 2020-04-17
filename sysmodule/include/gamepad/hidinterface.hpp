#pragma once

#include <switch.h>

namespace mc::controller {

	enum HidInterfaceType {
        HidInterfaceType_Bluetooth,
        HidInterfaceType_Usb
    };

	class HidInterface {

		public:
			HidInterface(HidInterfaceType type) : m_type(type) {};

			HidInterfaceType type(void) { return m_type; }

			virtual Result connect(void) = 0;
			virtual Result disconnect(void) = 0;
		
		private:
			HidInterfaceType m_type;

	};

}
