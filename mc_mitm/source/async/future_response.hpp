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

            void Wait(void) {
                os::WaitEvent(&m_ready_event);
            }

            bool TryWait(void) {
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
