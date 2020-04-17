#include <cstdio>
#include <cstring>
#include "gamepad/bluetoothdatabase.hpp"
#include "utils.hpp"

#include "logger.hpp"

namespace mc::controller {

	BluetoothDatabase::BluetoothDatabase() : m_location(BluetoothDatabase::default_location) {
		this->loadDatabase();
	}

	BluetoothDatabase::BluetoothDatabase(const char *location) : m_location(location) {
		this->loadDatabase();
	}

	Result BluetoothDatabase::loadDatabase(void) {
		FILE *fp = std::fopen(m_location, "rb");
		if (fp == nullptr) {
			//mc::log::Write("Error opening bluetooth database");
			return -1;
		}
		
		//mc::log::Write("Reading bluetooth database...");
		std::fread(&m_database, sizeof(m_database), 1, fp);
		std::fclose(fp);
		
		if (std::ferror(fp)) {
			//mc::log::Write("Error loading bluetooth database");
			return -1;
		}
		
		return 0;
	}

	Result BluetoothDatabase::storeDatabase(void) {	
		FILE *fp = std::fopen(m_location, "wb");
		if (fp == nullptr) {
			//mc::log::Write("Error opening bluetooth database");
			return -1;
		}
		
		//mc::log::Write("Writing bluetooth database...");
		std::fwrite(&m_database, sizeof(m_database), 1, fp);
		std::fclose(fp);
		
		if (std::ferror(fp)) {
			//mc::log::Write("Error writing bluetooth database");
			return -1;
		}
		
		return 0;
	}

	Result BluetoothDatabase::reload(void) {
		return this->loadDatabase();
	}

	Result BluetoothDatabase::addDevice(const BluetoothDevice *device) {
		if (device == nullptr) {
			//mc::log::Write("BluetoothDatabase::addDevice: device is nullptr");
			return -1;
		}
		
		/* Check if device is already in database */
		const BluetoothDevice *existing = this->search(&device->address);
		
		if (existing) {
			//mc::log::Write("Copying over exiting database entry...");
			/* Overwrite existing database entry */
			std::memcpy(const_cast<BluetoothDevice *>(existing), device, sizeof(BluetoothDevice));
		} 
		else if (m_database.size < BluetoothDatabase::max_devices) {
			//mc::log::Write("Adding new device to bluetooth database...");
			/* Add new device to the database */
			std::memcpy(&m_database.devices[m_database.size++], device, sizeof(BluetoothDevice));
		}
		else {
			return -1;
		}
		
		return this->storeDatabase();
	}

	Result BluetoothDatabase::removeDevice(const BluetoothAddress *address) {
		/* Locate index of device in database */
		uint8_t index = this->indexOf(address);
		if (index >= 0) {
			/* remove device from database and shift remaining devices down */
			std::memcpy(&m_database.devices[index], &m_database.devices[index+1], (m_database.size-index-1) * sizeof(BluetoothDevice));
			std::memset(&m_database.devices[--m_database.size], 0, sizeof(BluetoothDevice));
		}
		
		return this->storeDatabase();
	}

	const BluetoothDevice *BluetoothDatabase::search(const BluetoothAddress *address) {
		for (uint8_t i = 0; i < m_database.size; ++i) {
			if (bdcmp(address, &m_database.devices[i].address)) {
				return &m_database.devices[i];
			}
		}
		
		return nullptr;
	}

	const BluetoothDevice *BluetoothDatabase::operator[](uint8_t index) {
		return this->deviceAt(index);
	}

	const BluetoothDevice *BluetoothDatabase::deviceAt(uint8_t index) {
		if (index < m_database.size) {
			return &m_database.devices[index];
		}	
		
		return nullptr;
	}

	uint8_t BluetoothDatabase::indexOf(const BluetoothAddress *address) {
		for (uint8_t i = 0; i < m_database.size; ++i) {
			if (bdcmp(address, &m_database.devices[i].address)) {
				return i;
			}
		}

		return -1;
	}

	uint8_t BluetoothDatabase::size(void) {
		return m_database.size;
	}

}
