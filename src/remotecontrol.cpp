//+--------------------------------------------------------------------------
//
// File:        remotecontrol.cpp
//
// NightDriverStrip - (c) 2018 Plummer's Software LLC.  All Rights Reserved.
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
//
// Description:
//
//    Handles a simple IR remote for changing effects, brightness, etc.
//
// History:     Jun-14-2023     Rbergen        Extracted handle() from header
//---------------------------------------------------------------------------

#include "globals.h"

#if ENABLE_REMOTE

#include "systemcontainer.h"
#include "effects/strip/misceffects.h"

#define BRIGHTNESS_STEP 20

uint RemoteControl::getMinRepeatDelay(uint result, uint lastResult) const
{
    if (result == IR_OFF)
        return 0;               // Dim as fast as the remote sends it
    if (result == 0xFFFFFFFF)
        return 500;            // Repeats happen at 500ms
    if (result == lastResult)
        return 50;             // Manual presses get debounced to at least 50ms
    return 200;                // Default repeat delay
}

bool RemoteControl::processRepeatCode(uint& result, uint& lastResult)
{
    static uint lastRepeatTime = millis();
    
    if (0xFFFFFFFF != result && result != lastResult)
    {
        lastResult = result;
        return true;
    }

    uint minRepeatDelay = getMinRepeatDelay(result, lastResult);
    if (millis() - lastRepeatTime > minRepeatDelay)
    {
        debugV("Remote Repeat; lastResult == %08x, elapsed = %lu\n", lastResult, millis()-lastRepeatTime);
        result = lastResult;
        lastRepeatTime = millis();
        return true;
    }
    
    return false;
}

bool RemoteControl::handleCommandCode(uint result)
{
    auto &effectManager = g_ptrSystem->EffectManager();
    auto &deviceConfig = g_ptrSystem->DeviceConfig();

    auto it = std::find_if(std::begin(RemoteCommands), std::end(RemoteCommands),
        [result](const auto& cmd) { return cmd.code == result; });

    if (it != std::end(RemoteCommands))
    {
        it->handler(effectManager, deviceConfig);
        return true;
    }
    return false;
}

bool RemoteControl::handleColorCode(uint result)
{
    auto &effectManager = g_ptrSystem->EffectManager();
    
    auto it = std::find_if(std::begin(RemoteColorCodes), std::end(RemoteColorCodes),
        [result](const auto& colorCode) { return colorCode.code == result; });

    if (it != std::end(RemoteColorCodes))
    {
        debugI("Changing Color via remote: %08X\n", (uint32_t) it->color);
        effectManager.ApplyGlobalColor(it->color);
        
        #if FULL_COLOR_REMOTE_FILL
            auto effect = make_shared_psram<ColorFillEffect>("Remote Color", it->color, 1, true);
            if (effect->Init(g_ptrSystem->EffectManager().GetBaseGraphics()))
                effectManager.SetTempEffect(effect);
            else
                debugE("Could not initialize new color fill effect");
        #endif
        return true;
    }
    return false;
}

void RemoteControl::handle()
{
    static uint lastResult = 0;
    decode_results results;

    // Check if there's an IR code to process
    if (!_IR_Receive.decode(&results))
        return;

    uint result = results.value;
    _IR_Receive.resume();

    debugI("Received IR Remote Code: 0x%08X, Decode: %08X\n", result, results.decode_type);

    // Process repeat codes and validate
    if (!processRepeatCode(result, lastResult))
        return;

    // Try to handle as a command
    if (handleCommandCode(result))
        return;

    // Try to handle as a color code
    debugI("Looking for match for remote color code: %08X\n", result);
    handleColorCode(result);
}

#endif
