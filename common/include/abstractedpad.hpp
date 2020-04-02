#include <switch.h>
#include "virtualcontroller.hpp"

namespace mc::controller {

	class AbstractedPadVirtualController : public VirtualController {
		
		public:
			AbstractedPadVirtualController();
			~AbstractedPadVirtualController();
		
			Result connect(void);
			Result disconnect(void);
			Result setState(const SwitchProGamepadState* state);
		
		private:
			uint8_t						m_uniqueId;
			HiddbgAbstractedPadState	m_state;
	};

}
