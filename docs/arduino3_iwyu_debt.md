# Arduino3 IWYU Debt (Rolling List)

Goal: track include-order/IWYU issues discovered during Arduino3/Pioarduino churn. Keep this list small and tactical.

## Current Fixes (Already Applied)
- **`interfaces.h` Creation**: Extracted `IJSONSerializable`, `SettingSpec`, and `psram_allocator` helpers to a lightweight, standalone header. This allows system classes (`SystemContainer`, `EffectManager`) to decouple from the heavy `LEDStripEffect.h`.
- **`nd_network.h` Migration**: Renamed `network.h` to `nd_network.h` via `git mv` to resolve naming collisions with the Arduino framework's `<Network.h>`. Removed the `#include_next` hack.
- **Header Implementation Migration**: Relocated heavy method implementations from headers to source files for:
  - `LEDStripEffect`
  - `EffectManager`
  - `SoundAnalyzer` (refactored from template to class)
  - `DeviceConfig`
  - `NightDriverTaskManager`
- **JSON implementation Pimpl**: Hided `JSONWriter::WriterEntry` and `NetworkReader::ReaderEntry` implementation details in their respective `.cpp` files using `std::shared_ptr`.
- **Screen Hardware Firewall**: Unified hardware-specific Screen subclasses into `src/screen_impl.cpp`, keeping display library pollution (TFT_eSPI, LovyanGFX, etc.) out of the global scope.
- **Globals.h Thinning**: Moved `str_sprintf` and `types.h` includes out of the core `globals.h` to reduce the include graph for simple translation units.
- **Leaf Header Optimization**: Removed redundant `globals.h` includes from effects in `include/effects/` as they are guaranteed to be included into a context where `globals.h` was already parsed.

## Known IWYU Debt / Follow-ups
- **`RemoteDebug.h` usage**: Still currently included in `globals.h`. Planned for removal in favor of a native debug implementation.
- **`FastLED.h` dependency**: Central but heavy. Currently in `globals.h` because nearly everything touches `CRGB`.
- **ArduinoJson Template Fragility**: `ArduinoJson` types (`JsonObjectConst`) used in member function signatures or inline logic require the full header. `jsonserializer.h` currently provides this but remains heavy.

## Include Safety Rules (Arduino v3 Fragility)

These are pragmatic, minimal rules to avoid include-order landmines and maintain decoupling.

- **Use `interfaces.h`**: Prefer `interfaces.h` over `ledstripeffect.h` or `jsonserializer.h` for serialization types.
- **No Effects in Core**: `nd_network.h`, `systemcontainer.h`, and `taskmgr.h` must not include `ledstripeffect.h`.
- **No Heavy Displays in Core**: Keep `gfxbase.h`, `screen.h`, and driver headers out of core system headers.
- **Globals First**: `globals.h` must remain the first local include in any `.cpp` or non-leaf `.h` file to ensure feature macros are established early.

### Automation

- `python3 tools/audit_globals_order.py`: Verifies `globals.h` precedence and warns on missing includes in non-leaf files.
- `python3 tools/audit_include_rules.py`: Validates the include graph against the safety rules above.

## Build System Notes
- `platformio.ini` uses `build_src_flags` to optimize rebuild times.
- Verified builds on Av3 (`demo`, `hexagon`, `tinyled`) and Av2 (`mesmerizer`).
