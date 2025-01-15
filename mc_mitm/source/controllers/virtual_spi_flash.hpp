/*
 * Copyright (c) 2020-2025 ndeadly
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
#pragma once
#include <stratosphere.hpp>

namespace ams::controller {

    class VirtualSpiFlash {
        public:
            VirtualSpiFlash() {};
            ~VirtualSpiFlash();
            
            Result Initialize(const char *path);
            Result Read(int offset, void *data, size_t size);
            Result Write(int offset, const void *data, size_t size);
            Result SectorErase(int offset);
            Result CheckMemoryRegion(int offset, size_t size, bool *is_initialized);

        private:
            Result CreateFile(const char *path);
            Result EnsureMemoryRegion(int offset, const void *data, size_t size);
            Result EnsureInitialized();

            fs::FileHandle m_virtual_memory_file;
    };

}
