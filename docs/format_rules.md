# Format Specifier "Rules of the Road"

To ensure full compatibility with GCC 14 `-Wformat=2` across all ESP32 environments and future-proof for the Arduino3 migration, follow these rules project-wide. 

> [!NOTE]
> Always use explicit casts. Even if a type *should* be compatible on a specific platform, GCC 14 is pedantic about the exact underlying type mapping.

## The Rules

### 1. `size_t` and `sizeof()`
- **Rule**: Use `%zu` and cast the argument to `(size_t)`.
- **Reason**: `size_t` can vary in width (32/64 bit) and signedness/underlying type across compilers.
- **Example**: `str_sprintf("Size: %zu", (size_t)sizeof(buffer))`

### 2. `uint32_t` (and `uint`)
- **Rule**: Use `%lu` / `%lx` / `%lX` and cast to `(unsigned long)`.
- **Reason**: On many ESP32 partitions, `uint32_t` is defined as `unsigned long`, not `unsigned int`.
- **Example**: `debugI("Freq: %lu MHz", (unsigned long)ESP.getCpuFreqMHz())`

### 3. `int32_t`
- **Rule**: Use `%ld` and cast to `(long)`.
- **Reason**: Parallel to `uint32_t`, standardizing on `long` avoids ambiguity.
- **Example**: `debugW("RSSI: %ld", (long)WiFi.RSSI(i))`

### 4. Hexadecimal Colors (`CRGB`, `uint32_t`)
- **Rule**: Use `%08lX` (or `%06lX`) and cast to `(unsigned long)`.
- **Reason**: Colors are effectively 32-bit unsigned integers.
- **Example**: `debugV("Color: %08lX", (unsigned long)(uint32_t)color)`

### 5. `time_t`, `time_val` (Seconds and Micros)
- **Rule**: Use `%lld` and cast to `(long long)`.
- **Reason**: Arduino3/GCC 14 moves to 64-bit time to handle the 2038 problem.
- **Example**: `debugV("Time: %lld.%06lld", (long long)v.tv_sec, (long long)v.tv_usec)`

### 6. Small Integers (`uint16_t`, `uint8_t`, `byte`)
- **Rule**: Use `%d` and cast to `(int)`.
- **Reason**: These are promoted to `int` anyway, and `%d` is the most universal. If they are specifically unsigned-relevant, use `%u` and `(unsigned int)`.
- **Example**: `debugV("Count: %d", (int)count16)`

### 7. Core Macros (`debugV`, `debugI`, `debugW`, `debugE`)
- **Rule**: These internally use `printf` format strings. Apply the same rules above to the format strings provided to these macros.
