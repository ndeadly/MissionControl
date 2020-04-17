#include <algorithm>
#include "gamepad/controllermanager.hpp"
#include "gamepad/bluetoothinterface.hpp"
#include "utils.hpp"

#include "log.hpp"

namespace mc::controller {

	ControllerManager::ControllerManager() {

		if (hosversionAtLeast(7, 0, 0)) {
			Result rc = hiddbgAttachHdlsWorkBuffer();
			if R_FAILED(rc)
				fatalThrow(rc);
		}
		
		m_database = std::make_unique<BluetoothDatabase>();
	}

	ControllerManager::~ControllerManager() {

		if (hosversionAtLeast(7, 0, 0)) {
			Result rc = hiddbgReleaseHdlsWorkBuffer();
			if R_FAILED(rc)
				fatalThrow(rc);
		}
	}

	ControllerType ControllerManager::identify(uint16_t vid, uint16_t pid) {
		for (HardwareID hwId : SwitchProController::hardwareIds) {
			if ( (vid == hwId.vid) && (pid == hwId.pid) ) {
				return ControllerType_SwitchPro;
			}
		}

		for (HardwareID hwId : WiiUProController::hardwareIds) {
			if ( (vid == hwId.vid) && (pid == hwId.pid) ) {
				return ControllerType_WiiUPro;
			}
		}

		for (HardwareID hwId : WiimoteController::hardwareIds) {
			if ( (vid == hwId.vid) && (pid == hwId.pid) ) {
				return ControllerType_Wiimote;
			}
		}

		for (HardwareID hwId : Dualshock4Controller::hardwareIds) {
			if ( (vid == hwId.vid) && (pid == hwId.pid) ) {
				return ControllerType_Dualshock4;
			}
		}

		for (HardwareID hwId : XboxOneController::hardwareIds) {
			if ( (vid == hwId.vid) && (pid == hwId.pid) ) {
				return ControllerType_XboxOne;
			}
		}

		return ControllerType_Unknown;
	}

	ControllerType ControllerManager::identify(const HardwareID *hwId) {
		return ControllerManager::identify(hwId->vid, hwId->pid);
	}

	ControllerType ControllerManager::identify(const BluetoothDevice *device) {
		return ControllerManager::identify(device->vid, device->pid);
	}

	Result ControllerManager::registerBluetoothControllers(void) {
		Result rc;

		/* Register database entries with bluetooth module */
		for (uint8_t i = 0; i < m_database->size(); ++i) {
			rc = btdrvAddPairedDeviceInfo(m_database->deviceAt(i));
			if R_FAILED(rc)
				fatalThrow(rc);
		}

		//mc::log::Write("Registered pairing database with bluetooth module");
		return 0;
	}

	Result ControllerManager::attachBluetoothController(const BluetoothAddress *address) {

		const BluetoothDevice *device = m_database->search(address);
		if (device) {
			switch (this->identify(device)) {
				case ControllerType_SwitchPro:
					m_controllers.push_back(std::make_unique<SwitchProController>(HidInterfaceType_Bluetooth));
					break;
					
				case ControllerType_WiiUPro:
					m_controllers.push_back(std::make_unique<WiiUProController>(HidInterfaceType_Bluetooth));
					break;
					
				case ControllerType_Wiimote:
					m_controllers.push_back(std::make_unique<WiimoteController>(HidInterfaceType_Bluetooth));
					break;
					
				case ControllerType_Dualshock4:
				{
					m_controllers.push_back(std::make_unique<Dualshock4Controller>(HidInterfaceType_Bluetooth));

					/*
					// This definitely doesn't belong here 
					uint8_t r = 0xff;
					uint8_t g = 0x00;
					uint8_t b = 0x00;
						
					Dualshock4OutputReport0x11 report = {0xa2, 0x11, 0xc0, 0x20, 0xf3, 0x04, 0x00, 0x00, 0x00, r, g, b};
					report.crc = crc32Calculate(report.data, sizeof(report.data));
					
					BluetoothHidData hidData = {};
					hidData.length = sizeof(report) - 1;
					std::memcpy(&hidData.data, &report.data[1], hidData.length);

					Result rc = btdrvHidSetReport(address, HidReportType_OutputReport, &hidData);
					*/
				}
					break;
					
				case ControllerType_XboxOne:
					m_controllers.push_back(std::make_unique<XboxOneController>(HidInterfaceType_Bluetooth));
					break;
					
				default:
					//mc::log::Write("Unrecognised device: <%04x:%04x>", device->vid, device->pid);
					return -1;
			}

			// Pretty filthy hack until I work out how to structure this better
			m_controllers.back()->m_btInterface->setAddress(address);

			/*
			mc::log::Write("ControllerManager::attachBluetoothController: Attached bluetooth controller [%02x:%02x:%02x:%02x:%02x:%02x]",
				((uint8_t *)address)[0],
				((uint8_t *)address)[1],
				((uint8_t *)address)[2],
				((uint8_t *)address)[3],
				((uint8_t *)address)[4],
				((uint8_t *)address)[5]
			);
			*/

			return 0;
		}

		return -1;
	}

	Result ControllerManager::removeBluetoothController(const BluetoothAddress *address) {

		for (auto it = m_controllers.begin(); it < m_controllers.end(); ++it) {
			if ((*it)->m_btInterface != nullptr) {
				if (bdcmp(&(*it)->m_btInterface->address(), address)) {
					m_controllers.erase(it);

					/*
					mc::log::Write("ControllerManager::removeBluetoothController: Removed bluetooth controller [%02x:%02x:%02x:%02x:%02x:%02x]",
						((uint8_t *)address)[0],
						((uint8_t *)address)[1],
						((uint8_t *)address)[2],
						((uint8_t *)address)[3],
						((uint8_t *)address)[4],
						((uint8_t *)address)[5]
					);
					*/
					return 0;
				}
			}
		}

		/*
		mc::log::Write("ControllerManager::removeBluetoothController: Couldn't located controller [%02x:%02x:%02x:%02x:%02x:%02x]",
			((uint8_t *)address)[0],
			((uint8_t *)address)[1],
			((uint8_t *)address)[2],
			((uint8_t *)address)[3],
			((uint8_t *)address)[4],
			((uint8_t *)address)[5]
		);
		*/

		return -1;
	}

	void ControllerManager::removeControllers(void) {
		//mc::log::Write("Removing all controllers");
		m_controllers.clear();
	}

	Result ControllerManager::receiveBluetoothReport(const BluetoothAddress *address, const HidReport *report) {
		
		for (auto it = m_controllers.begin(); it < m_controllers.end(); ++it) {
			if ((*it)->m_btInterface != nullptr) {
				if (bdcmp(&(*it)->m_btInterface->address(), address)) {
					(*it)->receiveReport(report);
					return 0;
				}
			}
		}

		/*
		mc::log::Write("ControllerManager::receiveBluetoothReport: Controller not found [%02x:%02x:%02x:%02x:%02x:%02x] ",
			((uint8_t *)address)[0],
			((uint8_t *)address)[1],
			((uint8_t *)address)[2],
			((uint8_t *)address)[3],
			((uint8_t *)address)[4],
			((uint8_t *)address)[5]
		);
		*/

		return -1;
	}

}
