/*
 * Copyright (c) 2020-2022 ndeadly
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "virtual_spi_flash.hpp"
#include "switch_controller.hpp"

namespace ams::controller {

    namespace {

        constexpr size_t spi_flash_size = 0x10000;

        // Factory calibration data representing analog stick ranges that span the entire 12-bit data type in x and y
        SwitchAnalogStickFactoryCalibration lstick_factory_calib = {0xff, 0xf7, 0x7f, 0x00, 0x08, 0x80, 0x00, 0x08, 0x80};
        SwitchAnalogStickFactoryCalibration rstick_factory_calib = {0x00, 0x08, 0x80, 0x00, 0x08, 0x80, 0xff, 0xf7, 0x7f};

        // Stick parameters data that produce a 12.5% inner deadzone and a 5% outer deadzone (in relation to the full 12 bit range above)
        SwitchAnalogStickParameters default_stick_params = {0x0f, 0x30, 0x61, 0x00, 0x31, 0xf3, 0xd4, 0x14, 0x54, 0x41, 0x15, 0x54, 0xc7, 0x79, 0x9c, 0x33, 0x36, 0x63};

    }

    VirtualSpiFlash::~VirtualSpiFlash() {
        fs::CloseFile(m_virtual_memory_file);
    }

    Result VirtualSpiFlash::Initialize(const char *path) {
        // Check if the virtual spi flash file already exists and create it if not
        bool file_exists;
        R_TRY(fs::HasFile(&file_exists, path));
        if (!file_exists) {
            R_TRY(this->CreateFile(path));
        }

        // Open the virtual spi flash file for read and write
        R_TRY(fs::OpenFile(std::addressof(m_virtual_memory_file), path, fs::OpenMode_ReadWrite));

        // Make sure that all memory regions that we care about are initialised with defaults
        R_TRY(this->EnsureInitialized());

        return ams::ResultSuccess();
    }

    Result VirtualSpiFlash::Read(int offset, void *data, size_t size) {
        return fs::ReadFile(m_virtual_memory_file, offset, data, size);
    }

    Result VirtualSpiFlash::Write(int offset, const void *data, size_t size) {
        return fs::WriteFile(m_virtual_memory_file, offset, data, size, fs::WriteOption::Flush);
    }

    Result VirtualSpiFlash::SectorErase(int offset) {
        uint8_t buff[64];
        std::memset(buff, 0xff, sizeof(buff));

        // Fill sector at offset with 0xff
        unsigned int sector_size = 0x1000;
        for (unsigned int i = 0; i < (sector_size / sizeof(buff)); ++i) {
            R_TRY(fs::WriteFile(m_virtual_memory_file, offset, buff, sizeof(buff), fs::WriteOption::None));
            offset += sizeof(buff);
        }

        R_TRY(fs::FlushFile(m_virtual_memory_file));

        return ams::ResultSuccess();
    }

    Result VirtualSpiFlash::CheckMemoryRegion(int offset, size_t size, bool *is_initialized) {
        auto data = std::unique_ptr<uint8_t[]>(new uint8_t[size]());

        R_TRY(this->Read(offset, data.get(), size));
        for (size_t i = 0; i < size; ++i) {
            if (data[i] != 0xff) {
                *is_initialized = true;
                return ams::ResultSuccess();
            }
        }

        *is_initialized = false;
        return ams::ResultSuccess();
    }

    Result VirtualSpiFlash::CreateFile(const char *path) {
        // Create file representing first 64KB of SPI flash
        R_TRY(fs::CreateFile(path, spi_flash_size));

        R_TRY(fs::OpenFile(std::addressof(m_virtual_memory_file), path, fs::OpenMode_Write));
        ON_SCOPE_EXIT { fs::CloseFile(m_virtual_memory_file); };

        // Fill the file with 0xff
        uint8_t buff[64];
        std::memset(buff, 0xff, sizeof(buff));
        unsigned int offset = 0;
        while (offset < spi_flash_size) {
            size_t write_size = std::min(static_cast<size_t>(spi_flash_size - offset), sizeof(buff));
            R_TRY(fs::WriteFile(m_virtual_memory_file, offset, buff, write_size, fs::WriteOption::None));
            offset += write_size;
        }

        R_TRY(fs::FlushFile(m_virtual_memory_file));

        return ams::ResultSuccess();
    }

    Result VirtualSpiFlash::EnsureMemoryRegion(int offset, const void *data, size_t size) {
        bool initialized;
        R_TRY(this->CheckMemoryRegion(offset, size, &initialized));
        if (!initialized) {
            R_TRY(fs::WriteFile(m_virtual_memory_file, offset, data, size, fs::WriteOption::None));
        }

        return ams::ResultSuccess();
    }

    Result VirtualSpiFlash::EnsureInitialized() {
        const Switch6AxisCalibrationData factory_motion_calibration = {
            .acc_bias = {0, 0, 0},
            .acc_sensitivity = {16384, 16384, 16384},
            .gyro_bias = {0, 0, 0},
            .gyro_sensitivity = {13371, 13371, 13371}
        };
        R_TRY(this->EnsureMemoryRegion(0x6020, &factory_motion_calibration, sizeof(factory_motion_calibration)));

        const struct {
            SwitchAnalogStickFactoryCalibration lstick_factory_calib;
            SwitchAnalogStickFactoryCalibration rstick_factory_calib;
        } factory_stick_calibration = { lstick_factory_calib, rstick_factory_calib };
        R_TRY(this->EnsureMemoryRegion(0x603d, &factory_stick_calibration, sizeof(factory_stick_calibration)));

        const struct {
            RGBColour body;
            RGBColour buttons;
            RGBColour left_grip;
            RGBColour right_grip;
        } factory_colours = { {0x32, 0x32, 0x32}, {0xe6, 0xe6, 0xe6}, {0x46, 0x46, 0x46}, {0x46, 0x46, 0x46} };
        R_TRY(this->EnsureMemoryRegion(0x6050, &factory_colours, sizeof(factory_colours)));

        const Switch6AxisHorizontalOffset offset = {0, 0, 0};
        R_TRY(this->EnsureMemoryRegion(0x6080, &offset, sizeof(offset)));

        const struct {
            SwitchAnalogStickParameters lstick_default_parameters;
            SwitchAnalogStickParameters rstick_default_parameters;
        } factory_stick_parameters = { default_stick_params, default_stick_params };
        R_TRY(this->EnsureMemoryRegion(0x6086, &factory_stick_parameters, sizeof(factory_stick_parameters)));

        R_TRY(fs::FlushFile(m_virtual_memory_file));

        return ams::ResultSuccess();
    }

}
