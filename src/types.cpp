//+--------------------------------------------------------------------------
//
// File:        types.cpp
//
// NightDriverStrip - (c) 2024 Plummer's Software LLC.  All Rights Reserved.
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
// Description:
//
//    Utility functions like bit manipulation, string formatting, etc.
//
//---------------------------------------------------------------------------


#include "globals.h"

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <vector>

#include "effectmanager.h"
#include "ledstripeffect.h"
#include "types.h"

// str_sprintf - va-args style printf that returns the formatted string
//
// Originally inline in globals.h, moved here to reduce header bloat.

String str_sprintf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    va_list args_copy;
    va_copy(args_copy, args);

    // BUGBUG: Investigate a vasprintf here and String::copy() to get move semantics
    // on the return.
    // Could Save one complete format, a copy, and an alloc and we're called a
    // few times a second.
    int requiredLen = vsnprintf(nullptr, 0, fmt, args) + 1;
    va_end(args);

    if (requiredLen <= 0) {
        va_end(args_copy);
        return {};
    }

    std::unique_ptr<char []> str = std::make_unique<char []>(requiredLen);
    vsnprintf(str.get(), requiredLen, fmt, args_copy);
    va_end(args_copy);

    String retval;
    retval.reserve(requiredLen); // At least saves one scan of the buffer.

    retval = str.get();
    return retval;
}

// Memory deserialization helpers
//
// Originally inline in globals.h, moved here for consistency.

uint64_t ULONGFromMemory(const uint8_t * payloadData)
{
    return  (uint64_t)payloadData[7] << 56  |
            (uint64_t)payloadData[6] << 48  |
            (uint64_t)payloadData[5] << 40  |
            (uint64_t)payloadData[4] << 32  |
            (uint64_t)payloadData[3] << 24  |
            (uint64_t)payloadData[2] << 16  |
            (uint64_t)payloadData[1] << 8   |
            (uint64_t)payloadData[0];
}

uint32_t DWORDFromMemory(const uint8_t * payloadData)
{
    return  (uint32_t)payloadData[3] << 24  |
            (uint32_t)payloadData[2] << 16  |
            (uint32_t)payloadData[1] << 8   |
            (uint32_t)payloadData[0];
}

uint16_t WORDFromMemory(const uint8_t * payloadData)
{
    return  (uint16_t)payloadData[1] << 8   |
            (uint16_t)payloadData[0];
}

// formatSize - human-readable size formatting
//
// Originally inline in globals.h, moved here because it pulls in <sstream>.

String formatSize(size_t size, size_t threshold)
{
    // If the size is above the threshold, we want a precision of 2 to show more accurate value
    const int precision = size < threshold ? 0 : 2;

    const char* suffixes[] = {"", "K", "M", "G", "T", "P", "E", "Z"};
    size_t suffixIndex = 0;
    double sizeDouble = static_cast<double>(size);

    while (sizeDouble >= threshold && suffixIndex < (sizeof(suffixes) / sizeof(suffixes[0])) - 1)
    {
        sizeDouble /= 1000;
        ++suffixIndex;
    }

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(precision) << sizeDouble << suffixes[suffixIndex];
    std::string result = oss.str();  // Store the result to avoid dangling pointer
    return String(result.c_str());
}

// PreferPSRAMAlloc
//
// Will return PSRAM if it's available, regular ram otherwise
//
// Cache PSRAM availability and prefer it when allocating large buffers.

void * PreferPSRAMAlloc(size_t s)
{
    // Compute PSRAM availability once in a thread-safe way (I believe C++11+ guarantees thread-safe initialization of function-local statics).
    static const int s_psramAvailable = []() noexcept -> int {
        return psramInit() ? 1 : 0;
    }();

    if (s_psramAvailable)
    {
        debugV("PSRAM Array Request for %zu bytes\n", s);
        auto p = ps_malloc(s);
        if (!p)
        {
            debugE("PSRAM Allocation failed for %zu bytes\n", s);
            throw std::bad_alloc();
        }
        return p;
    }
    else
    {
        auto p = malloc(s);
        if (!p)
        {
            debugE("RAM Allocation failed for %zu bytes\n", s);
            throw std::bad_alloc();
        }
        return p;
    }
}
