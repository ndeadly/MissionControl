#pragma once

#include <switch.h>

namespace mc::controller {

	class BluetoothDatabase {
		
		public:
			static const uint8_t 	max_devices = 10;
			static constexpr const char * 	default_location = "sdmc:/config/MissionControl/bluetooth.db";

			BluetoothDatabase();
			BluetoothDatabase(const char *location);
			
			Result reload(void);
						
			Result addDevice(const BluetoothDevice *device);
			Result removeDevice(const BluetoothAddress *address);
		
			const BluetoothDevice *search(const BluetoothAddress *address);
			const BluetoothDevice *operator[](uint8_t index);
			const BluetoothDevice *deviceAt(uint8_t index);
			uint8_t                indexOf(const BluetoothAddress *address);
			
			uint8_t size(void);
				
		private:
			Result loadDatabase(void);
			Result storeDatabase(void);
					
			struct {
				uint8_t 		size;
				BluetoothDevice	devices[BluetoothDatabase::max_devices];
			} m_database;
			
			const char *	m_location;
			
	};

}
