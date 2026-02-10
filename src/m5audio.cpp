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

#include "globals.h"
#include "settings.h"

#if USE_M5
#include <M5Unified.h>
#include "m5audio.h"

namespace M5Audio {

bool MicRecord(int16_t* buffer, size_t samples, int frequency) {
    // M5 path unchanged; fills buffer with int16 samples
    return M5.Mic.record(buffer, samples, frequency, false);
}

void SpeakerSetVolume(uint8_t volume) {
    M5.Speaker.setVolume(volume);
}

void SpeakerEnd() {
    M5.Speaker.end();
}

void MicConfigAndBegin(int sampleRate) {
    auto cfg = M5.Mic.config();
    cfg.sample_rate = sampleRate;
    cfg.noise_filter_level = 0;
    cfg.magnification = 8;
    M5.Mic.config(cfg);
    M5.Mic.begin();
}

}
#endif
