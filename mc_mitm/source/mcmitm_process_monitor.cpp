/*
 * Copyright (c) 2020-2023 ndeadly
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
#include "mcmitm_process_monitor.hpp"

namespace ams::mc {

    namespace {

        ncm::ProgramId g_current_program = ncm::InvalidProgramId;
        os::Event g_process_switch_event(os::EventClearMode_AutoClear);

        Result _GetCurrentApplicationProgramId(ncm::ProgramId *program_id) {
            os::ProcessId process_id; 
            R_TRY(ams::pm::dmnt::GetApplicationProcessId(&process_id));
            R_TRY(ams::pm::dmnt::GetProgramId(program_id, process_id));

            R_SUCCEED();
        }

        Result GetCurrentApplicationProgramId(ncm::ProgramId *program_id) {
            R_TRY_CATCH(_GetCurrentApplicationProgramId(program_id)) {
                R_CATCH(ams::pm::ResultProcessNotFound) { *program_id = ncm::SystemAppletId::Qlaunch; }
            } R_END_TRY_CATCH;

            R_SUCCEED();
        }

    }

    os::Event *GetProcessSwitchEvent() {
        return &g_process_switch_event;
    }

    ncm::ProgramId GetCurrentProgramId() {
        return g_current_program;
    }

    void CheckForProcessSwitch() {
        ncm::ProgramId id;
        if (R_SUCCEEDED(GetCurrentApplicationProgramId(&id))) {
            if (id != g_current_program) {
                g_current_program = id;
                g_process_switch_event.Signal();
            }
        }
    }

}
