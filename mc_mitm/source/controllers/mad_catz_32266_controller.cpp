/*
 * Copyright (c) 2020-2026 ndeadly
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
#include "mad_catz_32266_controller.hpp"
#include <stratosphere.hpp>

namespace ams::controller {

    void MadCatz32266Controller::ProcessInputData(const bluetooth::HidReport *report) {
        auto madcatz_report = reinterpret_cast<const MadCatz32266ReportData *>(&report->data);

        switch(madcatz_report->id) {
            case 0x01:
                this->MapInputReport0x01(madcatz_report, report->size); break;
            case 0x02:
                this->MapInputReport0x02(madcatz_report); break;
            default:
                break;
        }
    }

    void MadCatz32266Controller::MapInputReport0x01(const MadCatz32266ReportData *src, u16 report_size) {
        enum DPadDirection {
            DPad_Released = 0,
            DPad_N,
            DPad_NE,
            DPad_E,
            DPad_SE,
            DPad_S,
            DPad_SW,
            DPad_W,
            DPad_NW
        };

        // The 0x01 Input Report has varying sizes based on the firmware
        // version of the C.T.R.L. R used.
        // The longer 16 byte report is the old firmware. ('Stock firmware')
        // The shorter 10 byte report is the new 125 firmware. ('XInput compatible')
        // Buttons and dpad in Input Report 0x01 are common to both firmware versions.
        // The 0x02 Input Report is unchanged between the two revisions.
        m_buttons.dpad_down  = (src->input0x01stock.buttons.dpad == DPad_S)  ||
                               (src->input0x01stock.buttons.dpad == DPad_SE) ||
                               (src->input0x01stock.buttons.dpad == DPad_SW);
        m_buttons.dpad_up    = (src->input0x01stock.buttons.dpad == DPad_N)  ||
                               (src->input0x01stock.buttons.dpad == DPad_NE) ||
                               (src->input0x01stock.buttons.dpad == DPad_NW);
        m_buttons.dpad_right = (src->input0x01stock.buttons.dpad == DPad_E)  ||
                               (src->input0x01stock.buttons.dpad == DPad_NE) ||
                               (src->input0x01stock.buttons.dpad == DPad_SE);
        m_buttons.dpad_left  = (src->input0x01stock.buttons.dpad == DPad_W)  ||
                               (src->input0x01stock.buttons.dpad == DPad_NW) ||
                               (src->input0x01stock.buttons.dpad == DPad_SW);

        m_buttons.A  = src->input0x01stock.buttons.B; // C.T.R.L. R has an Xbox One printed layout.
        m_buttons.B  = src->input0x01stock.buttons.A;
        m_buttons.X  = src->input0x01stock.buttons.Y;
        m_buttons.Y  = src->input0x01stock.buttons.X;

        m_buttons.R  = src->input0x01stock.buttons.RB; // Require that the player is using the Desktop mode switch
        m_buttons.ZR = src->input0x01stock.buttons.RT; // which has the triggers mapped to digital button bits.
        m_buttons.L  = src->input0x01stock.buttons.LB;
        m_buttons.ZL = src->input0x01stock.buttons.LT;

        m_buttons.minus = src->input0x01stock.buttons.back;
        m_buttons.plus  = src->input0x01stock.buttons.start;

        m_buttons.lstick_press = src->input0x01stock.buttons.L3;
        m_buttons.rstick_press = src->input0x01stock.buttons.R3;

        // Can't use the emblem button on C.T.R.L. R to map to the Switch home button.
        // It seems an obvious choice, but it's an 'instantaneous' on-off button,
        // and the Switch won't catch it.

        //m_buttons.home = src->input0x01stock.buttons.emblem;

        if (report_size <= 11) {
           m_left_stick  = PackAnalogStickValues(src->input0x01updated.left_stick.x,  InvertAnalogStickValue(src->input0x01updated.left_stick.y));
           m_right_stick = PackAnalogStickValues(src->input0x01updated.right_stick.x, InvertAnalogStickValue(src->input0x01updated.right_stick.y));
        } else {
           m_left_stick  = PackAnalogStickValues(src->input0x01stock.left_stick.x,  InvertAnalogStickValue(src->input0x01stock.left_stick.y));
           m_right_stick = PackAnalogStickValues(src->input0x01stock.right_stick.x, InvertAnalogStickValue(src->input0x01stock.right_stick.y));
        }
    }

    void MadCatz32266Controller::MapInputReport0x02(const MadCatz32266ReportData *src) {
        // C.T.R.L. R media button panel is: [ V- | REW | Play | FF | V+ ]
        // So map V- to Capture, and V+ to Home to match a Switch Pro controller.
        m_buttons.capture = src->input0x02.volume_down;
        m_buttons.home    = src->input0x02.volume_up;
    }
}
