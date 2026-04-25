# NightDriverStrip Agent Context

This file contains critical context, constraints, and architectural information for AI agents working on the NightDriverStrip codebase. **Read this file first.**

## 🚨 CRITICAL MANDATES & CONSTRAINTS

### 1. Safety & Operations
*   **DELETE Operations:** 🚨 **STOP and ask for permission** before ANY delete operations.
    *   ✅ **EXCEPTION:** Single files that you *just* created in the current session are OK to delete.
    *   ❌ **NEVER** delete multiple files, directories, system files, or project files without explicit user permission.
*   **File Modifications:**
    *   ❌ **NEVER** modify or add any files under `.pio/` or `.workspaces/`.
    *   **Do not** gratuitously change existing comments.
    *   **Do not** reformat existing files unless explicitly asked.
    *   **Do not** end lines with trailing whitespace.
    *   Use **UNIX-style newlines** (`\n`) for all files.

### 2. Directory Roaming (STRICT)
*   **Avoid the "Tummy Ache":** Do NOT read, index, or search inside hidden build directories (`.pio/`, `.workspaces/`, `.fbuild/`, `.gemini/`).
*   Roaming into these directories causes "agent state bloat" and results in a "woozy" feeling of giga-byte grep matches and multi-minute search delays. 
*   **ONLY** operate within `src/`, `include/`, `lib/`, `data/`, and `site/`.

### 3. OTA & Partition Stability
*   **MANDATORY Confirmation:** Every firmware build MUST call `ConfirmUpdate()` (which marks the app partition as valid) once it has successfully survived `setup()`.
*   Failure to do this will trigger the ESP32 rollback mechanism, leading to a "Groundhog Day" loop where the device reverts to old firmware on every reboot.

### 4. Hardware & Core Constraints
*   **Core Affinity:** 
    *   **Core 1:** Reserved for Drawing/Rendering loops.
    *   **Core 0:** Reserved for Network, Web Server, and OTA handling.
*   Keep rendering logic light and deterministic to prevent matrix stuttering.

---

## C++ Coding Standards
*   **Standard:** Uses **C++20** (`gnu++2a`). Prefer modern industry-standard C++ operations (containers, algorithms, `std::chrono`) over Arduino-specific wrappers.
*   **Exceptions:** ❌ Do not use try-catch blocks. Unrecoverable errors should call `throw std::runtime_error()`.
*   **Data Types:** ESP32 has hardware floating point but emulates double. **Prefer `float`** and `sinf()`, `cosf()`, `sqrtf()` over `double` variants.
*   **Memory:** Use `make_unique_psram<T>()` for large objects to keep internal SRAM free. Avoid large stack allocations.
*   **Concurrency:** Multi-threaded environment. Use `std::atomic` for flags. Thread-heavy locks should be rare; clear ownership is preferred.
*   **Style:**
    *   **Indentation:** 4 spaces.
    *   **Braces:** Allman style (scoping curlies on lines of their own).
    *   **Naming:** Classes/Methods (`PascalCase`), Locals (`camelCase`), Members (`_leadingUnderscoreCamelCase`).

---

## Project Overview & Architecture

### Key Components
*   **`GFXBase` (`include/gfxbase.h`):** The foundations for all graphical operations.
*   **`EffectManager` (`include/effectmanager.h`):** Manages transitions and effect lifecycles.
*   **`HUB75GFX` / `WS281xGFX`:** Hardware-specific drivers for Matrix and Strip layouts.

### Main Process Flow
`main.cpp` initializes hardware in `setup()`, starts the secondary threads, and enters a `loop()` that monitors system health. The `EffectManager` drives the active effect's `Draw()` method inside the drawing thread on Core 1.

### Development Guides
*   **"No-Net" Compatibility:** Always verify that core logic compiles with `-DENABLE_WIFI=0`. Use the `demo` environment for this.
*   **Adding Effects:** Implement new effects directly in their header files (`include/effects/matrix/PatternNewEffect.h`). Do NOT create corresponding .cpp files. Alphabetize includes in the header.
*   **BlinkenPerBit:** Maximize visual improvement ("blinken") while minimizing source code complexity ("bits").

---

## Key Files Reference
*   `platformio.ini`: Central build configuration.
*   `include/globals.h`: Feature flags and manifest constants.
*   `src/effects.cpp`: The central registry of all available visual effects.
*   `CONTRIBUTING.md`: Detailed style and contribution guidelines.
