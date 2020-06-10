#pragma once

#include <switch.h>
#include "virtualcontroller.hpp"

namespace controller {

	class HdlsVirtualController : public VirtualController  {
		
		public:
			HdlsVirtualController();
			~HdlsVirtualController();
			
			Result connect(void);
			Result disconnect(void);
			Result setState(const SwitchProGamepadState* state);
			
		private:
			uint64_t                m_handle;
			HiddbgHdlsDeviceInfo    m_device; 
			HiddbgHdlsState         m_state;
	};


}
