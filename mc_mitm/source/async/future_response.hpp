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

namespace ams {

    template <class T1, class T2, class T3>
    class FutureResponse {
        public:
            FutureResponse(T1 type) : m_type(type) {
                os::InitializeEvent(&m_ready_event, false, os::EventClearMode_AutoClear);
            }

            ~FutureResponse() {
                os::FinalizeEvent(&m_ready_event);
            }            
            
            const T1& GetType() {
                return m_type;
            }

            void SetData(const T2& data) {
                m_data = data;
                os::SignalEvent(&m_ready_event);
            }

            const T2& GetData() {
                return m_data;
            }

            void SetUserData(const T3& user_data) {
                m_user_data = user_data;
            }

            const T3& GetUserData() {
                return m_user_data;
            }

            void Wait() {
                os::WaitEvent(&m_ready_event);
            }

            bool TryWait() {
                return os::TryWaitEvent(&m_ready_event);
            }

            bool TimedWait(TimeSpan timeout) {
                return os::TimedWaitEvent(&m_ready_event, timeout);
            }

        private:
            T1 m_type;
            T2 m_data;
            T3 m_user_data;
            os::EventType m_ready_event;
    };

}
