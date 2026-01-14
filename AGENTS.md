# NightDriverStrip Agent Context

This file contains critical context, constraints, and architectural information for AI agents working on the NightDriverStrip codebase. **Read this file first.**

## 🚨 CRITICAL MANDATES & CONSTRAINTS

**Safety & Operations**
*   **DELETE Operations:** 🚨 **STOP and ask for permission** before ANY delete operations.
    *   ✅ **EXCEPTION:** Single files that you *just* created in the current session are OK to delete.
    *   ❌ **NEVER** delete multiple files, directories, system files, or project files without explicit user permission.
*   **File Modifications:**
    *   ❌ **NEVER** modify or add any files under `.pio/`.
    *   **Do not** gratuitously change existing comments.
    *   **Do not** reformat existing files unless explicitly asked. Code styles are inconsistent; match the *local* neighborhood of the code you are editing.
    *   **Do not** end lines with trailing whitespace.
    *   Use **UNIX-style newlines** (`\n`) for all files.
*   **Consult `.md` files in the top-level directory and docs/.**

**C++ Coding Standards**
*   **Standard:** Uses **C++20**, but defers to Arduino conventions for compatibility. Prefer industry-standard C++ operations for containers, summations, time, etc. over Arduino.
*   **Exceptions:** ❌ **Do not use try-catch blocks or C++ exceptions.** Unrecoverable, fatal errors should call `throw std::runtime_error()`.
*   **Data Types:** ESP32 has hardware floating point but emulates double. **Prefer `float`** data types and function calls over `double`.
*   **Memory:**
    *   ESP32 Targets vary between 320K and 16MB (PSRAM) of RAM.
    *   Use `make_unique_psram<T>()` for large object allocations to prefer PSRAM when available.
    *   Avoid large stack allocations.
*   **Concurrency:** Multiple tasks run on potentially multiple cores. The design generally assumes only one task clearly owns each data. There are very few `std::lock_guard` or `std::mutex` locks needed.
*   **Best Practices:** Use C++20 best practices, like returning `std::optional`, relying upon RVO and NVRO, not using in/out parameters. Prefer constant, smart data structures and containers and `<algorithm>` over manually coded loops.
*   **Style:**
    *   Code mostly follows Microsoft C++ style guide. Match format to neighboring code in the same file or sibling files.
    *   **Indentation:** 4 spaces.
    *   **Braces:** Scoping curlies are on lines of their own.
    *   **Naming:** Classes/Methods (`PascalCase`), Local variables (`camelCase`), Class member variables (`_leadingUnderscoreCamelCase`).
    *   **Function/Method Arguments & Returns:**
        *   Error codes: `bool function() { return false; }` or custom error enums.
        *   `std::optional<T>` for functions that may not return a value.
        *   Prefer early error returns instead of deeply nested if-statements.
*   **Enums:** Prefer `class enum` to bare `enum`.
*   **Explicit Naming:** For structure members, typedefs, enum members, return types, etc., use explicitly named values.

**Logging:** Use macros `debugV`, `debugE`, `debugI`, `debugW`, `debugF`, `debugT`, `debugD` for verbose, error, informational, warning, fatal, trace, debug respectively. These accept variadic C++ `std::printf` format strings and should be used sparingly.

---


## Project Overview

NightDriverStrip is a comprehensive C++ firmware framework for the ESP32 microcontroller designed to drive various LED displays, including WS2812B strips and HUB75 matrices. It features audio-reactive effects, Wi-Fi connectivity, a built-in web server with a React-based frontend, and OTA (Over-The-Air) update capabilities.

The project is structured to support multiple hardware configurations (environments) through PlatformIO, ranging from simple LED strips (`demo`) to complex matrix displays (`mesmerizer`).

### Key Technologies

*   **Firmware:** C++20 (via `gnu++2a` flag), Arduino Framework, PlatformIO.
*   **Frontend:** React, Vite, Material UI (located in `site/`).
*   **Hardware Support:** ESP32, ESP32-S3, ESP32-C3 (M5StickC, M5Stack, Heltec, Custom boards).
*   **Graphics Engine:** Custom `GFXBase` extending `Adafruit_GFX`, with specialized drivers (`WS281xGFX`, `HUB75GFX`).
*   **Libraries:** FastLED, SmartMatrix, ArduinoJson, ESPAsyncWebServer.

## Directory Structure

*   `src/`: C++ source files (`main.cpp`, `effects.cpp`, etc.).
*   `include/`: Header files (`globals.h`, `gfxbase.h`, `effects.h`).
*   `site/`: React-based web frontend source code.
*   `tools/`: Python and Shell scripts for build automation and utilities.
*   `data/`: SPIFFS data (e.g., `effects.cfg`).
*   `platformio.ini`: PlatformIO project configuration file, defining all build environments.

---


## Architecture & Core Components (Refer to `CODEBASE_INTRO.md` for more detail)

**Threading Model**
The application is multi-threaded. Key threads include:
1.  **Main Thread (`main.cpp`):** Initializes hardware/software, then starts other threads.
2.  **Drawing Thread (`drawing.cpp`):** Manages continuous updates and effect transitions on the LED display.
3.  **Audio Thread (`audio.cpp`):** Captures and processes audio input, influencing LED animations.
4.  **Network Thread (`network.cpp`):** Handles core network tasks, WiFi, scheduled network operations.
5.  **Remote Control Thread (`remotecontrol.cpp`):** Processes commands from IR remote controls.
6.  **Socket Server Thread (`socketserver.cpp`):** Listens for and processes incoming socket connections.
7.  **Web Server Thread (`webserver.cpp`):** Serves web pages, listens for HTTP requests for control and information.

**Key Classes**
*   **`GFXBase` (`include/gfxbase.h`):** The foundational class for all graphical operations, extending `Adafruit_GFX`. It provides core drawing primitives and palette management.
*   **`EffectManager` (`include/effectmanager.h`):** Responsible for creating, managing, and transitioning visual effects.
*   **`LEDStripEffect` (`include/ledstripeffect.h`):** The base class from which all individual LED effects derive.
*   **`HUB75GFX` / `WS281xGFX`:** Specialized `GFXBase` extensions for HUB75 matrix and WS2812B strip arrangements, respectively.

**Main Process Flow**
The application starts in `setup()` in `main.cpp` (hardware/software init, thread creation). It then enters a `loop()` that primarily monitors system status. The core logic of animation and network communication runs in dedicated threads. The `EffectManager`'s `Update()` method is called within the drawing loop, which in turn calls the `Draw()` method of the currently active effect.

---


---


## Interactive Debugging & CLI

The project now features a robust, interactive Command Line Interface (CLI) accessible via the Serial port (115200 baud).

### Key Features
*   **Fuzzy Effect Selection:** Users can type partial names (e.g., `smok`) to match effects (e.g., `PatternSMSmoke`).
*   **Tab Completion:** Supports tab-completion for commands and effect names.
*   **Brightness Nudging:** The `brightness` command supports relative adjustment (`+`, `++`, `-`, `--`) in addition to absolute values.
*   **Architecture:** core CLI logic resides in `src/debug_cli.cpp` and `include/debug_cli.h` within the `DebugCLI` namespace.  **Avoid adding CLI logic to `main.cpp`.**

### Common Commands
*   `help`: List available commands.
*   `list`: List all available effects.
*   `set <effect_name|index>`: Switch to a specific effect (fuzzy matching supported).
*   `brightness <0-255|++|-->`: Adjust global brightness.
*   `wifi <ssid> <password>`: Set WiFi credentials (if WiFi is enabled).

---


## Development Guides

### Build Environment Strategy
Choosing the right environment saves time:
*   **`demo` (Lightweight):** fast iteration, no WiFi, no Audio. Use this for general logic, effects development, and verifying "no-net" compatibility.
*   **`mesmerizer` (Full Integration):** Heavyweight, includes WiFi, Audio, and Matrix support. Use for final integration testing and hardware-specific features.
*   *Note: There is currently no native (PC-host) build environment.*

### Code Stability & Networking
*   **"No-Net" Compatibility:** Always verify that core logic compiles with `-DENABLE_WIFI=0`. Use the `demo` environment to verify this.
*   **Guards:** Guard all network-specific code (API calls, socket handling) with `#if ENABLE_WIFI` or `#if INCOMING_WIFI_ENABLED` to prevent linker errors in offline builds.

### Dependency Management
*   **Scrutinize `lib_deps`:** Do not add external libraries to `platformio.ini` without checking if a simple, local implementation is sufficient. (See **BlinkenPerBit** philosophy).

### Adding New Effects
*   **Do not modify existing files in `effects/matrix/`.**
*   **Do not modify `gfxbase.h`.**
*   `MATRIX_WIDTH` / `MATRIX_HEIGHT` and other manifests are `#define`d by `globals.h`, which is included upstream.
*   Implement new effects directly in their header files (e.g., `include/effects/matrix/PatternNewEffect.h`). **Implement NO corresponding `.cpp` file.**
*   Do not renumber existing entries in `include/effects.h`. Put new MESMERIZER effects in the existing block. Most new effects go in the STRIP block.
*   Generously use color utilities from `FastLED.h` and math functions from `FastLED`'s `lib8tion.h`.

### Configuration
*   **Feature Flags:** Defined in `platformio.ini` as `build_flags` (e.g., `-DENABLE_WIFI=1`) or directly in `include/globals.h`.
*   **Global Settings:** `include/globals.h` contains many default macro definitions for various features and hardware settings. Consider `custom_globals.h` to override if needed.

---


## Building & Tools

### PlatformIO Commands
*   **Build Firmware:** `pio run -e [environment_name]` (e.g., `pio run -e demo`).
*   **Upload Firmware:** `pio run -e [environment_name] --target upload`.
*   **Build & Upload Filesystem (SPIFFS):** `pio run -e [environment_name] --target uploadfs`.
*   **Static Analysis:** `pio check`.

### Useful Helper Scripts (`tools/`)
*   **List Available Targets (PlatformIO Environments):**
    ```bash
    python3 tools/show_envs.py
    ```
    (Outputs a JSON array of available environments)
*   **Show Configuration Options for a Target:**
    ```bash
    python3 tools/show_features.py [environment_name]
    ```
    (Outputs a JSON array of set configuration options for the specified environment)
*   **Build All Configurations (Smoke Test):**
    ```bash
    python tools/build_all.py
    ```

### Frontend (Web UI) Build
The web interface source is in `site/`.
1.  Navigate to the `site/` directory.
2.  Install dependencies: `npm install`.
3.  Build web assets: `npm run build`.
    *Note: Built assets are automatically embedded into the firmware during the PlatformIO build process via `tools/bake_site.py`.*

---


## API & Connectivity

(Refer to `REST_API.md` for full details.)

*   **REST-like API:** Exposed when WiFi and webserver are enabled. Provides endpoints to:
    *   Retrieve device and effect information.
    *   Change current effect, enable/disable, move, copy, delete effects.
    *   Get/set device and effect configuration settings (with/without validation).
    *   Reset configuration or restart the device.
*   **WebSockets:**
    *   `/ws/effects`: Pushes events for effect list updates (e.g., active effect changes, enabled state changes).
    *   `/ws/effectframes`: Pushes color data packets as the LED display updates (for real-time visualization).

---


## Project Philosophy (Refer to `CONTRIBUTING.md` for full details)

*   **BlinkenPerBit Metric:** The core philosophy for contributions. Maximize functional improvement (blinken) while minimizing impact on source code complexity and length (bits).
*   **Contribute to Main:** Prefer contributing features and fixes to the main repository (`PlummersSoftwareLLC/NightDriverStrip`) rather than maintaining separate forks, to benefit the wider community.
*   **Local Consistency:** When contributing, work in the style of the function, class, or file you are modifying. Minimize changes to existing, inconsistent styles. Stylistic changes should be discussed first.

---


## Key Files Reference

*   `AGENTS.md`: This file.
*   `platformio.ini`: The central configuration for all PlatformIO build environments.
*   `include/globals.h`: Global system constants, feature flags, and versioning.
*   `src/main.cpp`: The application's entry point, orchestrating hardware initialization and task scheduling.
*   `src/effects.cpp`: The "Registry" for all visual effects, where they are enabled/disabled for specific builds.
*   `include/gfxbase.h`: The core graphics abstraction layer, extending `Adafruit_GFX` to provide drawing primitives for the LED displays.
*   `CODEBASE_INTRO.md`: A human-readable introduction to the codebase, including detailed explanations of key components and threads.
*   `CONTRIBUTING.md`: Detailed guidelines for contributing to the project, emphasizing the "BlinkenPerBit" metric and code style expectations.
*   `REST_API.md`: Documentation for the on-board REST-like API endpoints and WebSocket interfaces.
