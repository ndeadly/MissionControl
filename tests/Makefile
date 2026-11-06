CXX ?= g++
BUILD_DIR ?= /tmp/missioncontrol-tests
CXXFLAGS ?= -std=c++20 -O2 -g -Wall -Wextra -Werror -pthread

.PHONY: test
test:
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) ble_regression.cpp -o $(BUILD_DIR)/ble_regression
	$(BUILD_DIR)/ble_regression
	$(CXX) $(CXXFLAGS) -I../lib/libnx/nx/include advertising_regression.cpp -o $(BUILD_DIR)/advertising_regression
	$(BUILD_DIR)/advertising_regression

	mkdir -p $(BUILD_DIR)/gatt
	cp ../mc_mitm/source/ble/ble_gatt.cpp ../mc_mitm/source/ble/ble_gatt.hpp gatt_shim/* $(BUILD_DIR)/gatt/
	$(CXX) $(CXXFLAGS) -I$(BUILD_DIR)/gatt gatt_regression.cpp -o $(BUILD_DIR)/gatt_regression
	$(BUILD_DIR)/gatt_regression

	mkdir -p $(BUILD_DIR)/profile
	cp ../mc_mitm/source/ble/ble_profile.cpp ../mc_mitm/source/ble/ble_profile.hpp profile_shim/* $(BUILD_DIR)/profile/
	$(CXX) $(CXXFLAGS) -I$(BUILD_DIR)/profile profile_regression.cpp -o $(BUILD_DIR)/profile_regression
	$(BUILD_DIR)/profile_regression

	mkdir -p $(BUILD_DIR)/sleep
	cp ../mc_mitm/source/ble/ble_sleep.cpp ../mc_mitm/source/ble/ble_sleep.hpp profile_shim/* sleep_shim/* $(BUILD_DIR)/sleep/
	$(CXX) $(CXXFLAGS) -I$(BUILD_DIR)/sleep sleep_regression.cpp -o $(BUILD_DIR)/sleep_regression
	$(BUILD_DIR)/sleep_regression
