//+--------------------------------------------------------------------------
//
// File:        ledstripeffect.cpp
//
// NightDriverStrip - (c) 2018-2026 Plummer's Software LLC.  All Rights Reserved.
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
//    along with Nightdriver.  It is normally found in copying.txt
//    If not, see <https://www.gnu.org/licenses/>.
//
// Description:
//
//    Defines the base class for effects that run locally on the chip
//
// History:     Apr-13-2019         Davepl      Created for NightDriverStrip
//
//---------------------------------------------------------------------------

#include "globals.h"
#include "effectmanager.h"
#include "jsonserializer.h"
#include "ledstripeffect.h"

#ifdef HEXAGON
#include "ws281xgfx.h"
#endif

// Static member initialization
INIT_EFFECT_SETTING_SPECS(LEDStripEffect, _baseSettingSpecs);

#if HEXAGON
std::shared_ptr<HexagonGFX> LEDStripEffect::hg(size_t channel)
{
    return std::static_pointer_cast<HexagonGFX>(_GFX[channel]);
}
#endif

void LEDStripEffect::FillBaseSettingSpecs()
{
    // If the base SettingSpec instances already exist, bail out...
    if (!_baseSettingSpecs.empty())
        return;

    // ...otherwise, create and add them

    _baseSettingSpecs.emplace_back(
        ACTUAL_NAME_OF(_friendlyName),
        "Friendly name",
        "The friendly name of the effect, as shown in the web UI and/or on the matrix.",
        SettingSpec::SettingType::String
    );
    _baseSettingSpecs.emplace_back(
        ACTUAL_NAME_OF(_maximumEffectTime),
        "Maximum effect time",
        "The maximum time in ms that the effect is shown per effect rotation. This duration is only applied if it's "
        "shorter than the default effect interval. A value of 0 means no maximum effect time is set.",
        SettingSpec::SettingType::PositiveBigInteger
    );
    _baseSettingSpecs.emplace_back(
        "hasMaximumEffectTime",
        "Has maximum effect time set",
        "Indicates if the effect has a maximum effect time set.",
        SettingSpec::SettingType::Boolean
    ).Access = SettingSpec::SettingAccess::ReadOnly;
    _baseSettingSpecs.emplace_back(
        "clearMaximumEffectTime",
        "Clear maximum effect time",
        "Clear maximum effect time. Set to true to reset the maximum effect time to the default value.",
        SettingSpec::SettingType::Boolean
    ).Access = SettingSpec::SettingAccess::WriteOnly;
}

bool LEDStripEffect::SerializeToJSON(JsonObject& jsonObject)
{
    auto jsonDoc = CreateJsonDocument();

    jsonDoc[PTY_EFFECTNR]       = static_cast<int>(effectId());
    jsonDoc["fn"]               = _friendlyName;
    jsonDoc["es"]               = _enabled ? 1 : 0;

    // Migrations are done when the effect is constructed from JSON, so by definition all known
    // migrations have been performed by the time we get here.
    jsonDoc["mi"]               = to_value(JSONMigrations::All);

    // Only add the max effect time and core effect flag if they're not the default, to save space
    if (HasMaximumEffectTime())
        jsonDoc["mt"]           = _maximumEffectTime;
    if (_coreEffect)
        jsonDoc[PTY_COREEFFECT] = 1;

    return SetIfNotOverflowed(jsonDoc, jsonObject, __PRETTY_FUNCTION__);
}

bool LEDStripEffect::SerializeSettingsToJSON(JsonObject& jsonObject)
{
    auto jsonDoc = CreateJsonDocument();

    jsonDoc[ACTUAL_NAME_OF(_friendlyName)] = _friendlyName;
    jsonDoc[ACTUAL_NAME_OF(_maximumEffectTime)] = _maximumEffectTime;
    jsonDoc["hasMaximumEffectTime"] = HasMaximumEffectTime();

    return SetIfNotOverflowed(jsonDoc, jsonObject, __PRETTY_FUNCTION__);
}

bool LEDStripEffect::SetSetting(const String& name, const String& value)
{
    RETURN_IF_SET(name, ACTUAL_NAME_OF(_friendlyName), _friendlyName, value);
    RETURN_IF_SET(name, ACTUAL_NAME_OF(_maximumEffectTime), _maximumEffectTime, value);

    bool clearMaximumEffectTime = false;
    if (SetIfSelected(name, "clearMaximumEffectTime", clearMaximumEffectTime, value))
    {
        if (clearMaximumEffectTime)
            _maximumEffectTime = 0;

        return true;
    }

    return false;
}
