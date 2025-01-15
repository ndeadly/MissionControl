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
#include "async.hpp"

namespace ams::async {

    namespace {

        constexpr size_t ThreadCount = 1;
        constexpr size_t ThreadStackSize = 0x2000;
        constexpr s32 ThreadPriority = 10;

        alignas(os::MemoryPageSize) constinit u8 g_thread_stacks[ThreadCount][ThreadStackSize];
        constinit os::ThreadType g_thread_pool[ThreadCount];

        constexpr size_t MessageBufferSize = 32;
        constinit uintptr_t g_message_buffer[MessageBufferSize];
        constinit os::MessageQueueType g_work_queue;

        void WorkerThreadFunc(void *) {
            uintptr_t ptr;
            for (;;) {
                os::ReceiveMessageQueue(&ptr, &g_work_queue);

                // Convert pointer to work function back to correct type and claim ownership
                auto work_func = std::unique_ptr<AsyncFunction>(reinterpret_cast<AsyncFunction *>(ptr));

                // Execute the work function
                (*work_func)();
            }
        }

    }

    Result Initialize() {
        os::InitializeMessageQueue(&g_work_queue, g_message_buffer, MessageBufferSize);

        for (unsigned int i = 0; i < ThreadCount; ++i) {
            R_TRY(os::CreateThread(&g_thread_pool[i],
                WorkerThreadFunc,
                nullptr,
                g_thread_stacks[i],
                ThreadStackSize,
                ThreadPriority
            ));

            os::SetThreadNamePointer(&g_thread_pool[i], "mc::AsyncWorker");
            os::StartThread(&g_thread_pool[i]);
        }

        R_SUCCEED();
    }

    void Finalize() {
        os::FinalizeMessageQueue(&g_work_queue);

        for (unsigned int i = 0; i < ThreadCount; ++i) {
            os::DestroyThread(&g_thread_pool[i]);
        }

    }

    void QueueWork(AsyncFunction *function) {
        os::SendMessageQueue(&g_work_queue, reinterpret_cast<uintptr_t>(function));
    }

}
