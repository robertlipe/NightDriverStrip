//+--------------------------------------------------------------------------
//
// NightDriverStrip - (c) 2026 Plummer's Software LLC.  All Rights Reserved.
//
// This file is part of the NightDriver software project.
//
//    NightDriver is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    NightDriver is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with Nightdriver.  It is normally found in copying.txt
//    If not, see <https://www.gnu.org/licenses/>.
//
//---------------------------------------------------------------------------

#pragma once

#include "globals.h"
#include "effectmanager.h"
#include "ledstripeffect.h"

#if USE_M5
#include <stddef.h>
#include <stdint.h>

namespace M5Audio {
    bool MicRecord(int16_t* buffer, size_t samples, int frequency);
    void SpeakerSetVolume(uint8_t volume);
    void SpeakerEnd();
    void MicConfigAndBegin(int sampleRate);
}
#endif
