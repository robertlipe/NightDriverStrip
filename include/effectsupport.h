//+--------------------------------------------------------------------------
//
// File:        effectsupport.h
//
// NightDriverStrip - (c) 2023 Plummer's Software LLC.  All Rights Reserved.
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
//    Declarations of different types for the effect initializers included
//    in effects.cpp.
//
// History:     Sep-26-2023         Rbergen     Created for NightDriverStrip
//
//---------------------------------------------------------------------------

#pragma once

#include "globals.h"
#include <type_traits>
#include <string_view>
#include <cstdint>
#include <algorithm>
#include <vector>
#include <utility>
#include "hashing.h"

// Palettes used by a number of effects - now in colordata.h / colordata.cpp


// A pointer to the global effect factories.
extern std::unique_ptr<EffectFactories> g_ptrEffectFactories;

// ------------------------------------------------------------
// Support for building stable factory IDs and combining them
// ------------------------------------------------------------

// A type alias for removing const, volatile, and reference from a type.
template<typename T>
using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

// Gets the effect ID of a given effect type.
template<typename T>
constexpr EffectId effect_id_of_type() {
    static_assert(std::is_base_of_v<LEDStripEffect, remove_cvref_t<T>>,
                  "Type must derive from EffectWithId<Id>");
    return remove_cvref_t<T>::ID;   // compile-time constant
}

// Build a stable 64-bit ID for a factory based on effect type and ctor args
// Note that this is declared as a constexpr function, which means all ctor
// args need to be static types. The fact this code compiles confirms that
// is indeed the case.
template<typename TEffect, typename... Args>
constexpr FactoryId factory_id_of_instance(const Args&... args)
{
    FactoryId h = fnv1a::hash<FactoryId>("effect");
    h = fnv1a::hash(effect_id_of_type<TEffect>(), h);
    h = fnv1a::hash_pack(h, args...);
    return h;
}

// Adds a default and JSON effect factory for a specific effect number and type.
// All parameters beyond effectNumber and effect type are forwarded to the default constructor.
template<typename TEffect, typename... Args>
inline EffectFactories::NumberedFactory& AddEffect(EffectFactories& factories, Args&&... args)
{
    return factories.AddEffect(
        effect_id_of_type<TEffect>(),
        [=]() -> std::shared_ptr<LEDStripEffect> { return make_shared_psram<TEffect>(args...); },
        [](const JsonObjectConst& jsonObject) -> std::shared_ptr<LEDStripEffect> { return make_shared_psram<TEffect>(jsonObject); },
        factory_id_of_instance<TEffect>(args...)
    );
}


// Fold-expression helper to register many at once with brief syntax:
//   RegisterAll(*g_ptrEffectFactories,
//       Effect<idStripPalette, MyEffect>(args...),
//       Disabled(Effect<idStripColorFill, OtherEffect>(args...)));
template<typename... Adders>
inline void RegisterAll(EffectFactories& factories, Adders&&... adders)
{
    (static_cast<void>(adders(factories)), ...);
}

// Builder for a single effect entry used with RegisterAll
template<typename TEffect, typename... Args>
inline auto Effect(Args&&... args)
{
    return [=](EffectFactories& factories) -> EffectFactories::NumberedFactory&
    {
        return AddEffect<TEffect>(factories, args...);
    };
}

// Decorator to mark an entry disabled-on-load when using RegisterAll
template<typename F>
inline auto Disabled(F adder)
{
    return [=](EffectFactories& factories) -> EffectFactories::NumberedFactory&
    {
        auto& nf = adder(factories);
        nf.LoadDisabled = true;
        return nf;
    };
}

// Defines used by some StarEffect instances

constexpr float kStarEffectProbability = 1.0f;
constexpr float kStarEffectMusicFactor = 1.0f;
