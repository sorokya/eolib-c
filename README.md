# EOLib

[![Build Status][actions-badge]][actions-url]
[![Docs][docs-badge]][docs-url]
[![License][mit-badge]][mit-url]

[docs-badge]: https://img.shields.io/github/actions/workflow/status/sorokya/eolib-c/docs.yml?label=docs
[docs-url]: https://sorokya.github.io/eolib-c/
[mit-badge]: https://img.shields.io/badge/license-MIT-blue.svg
[mit-url]: https://github.com/sorokya/eolib-c/blob/master/LICENSE
[actions-badge]: https://github.com/sorokya/eolib-c/actions/workflows/build.yml/badge.svg
[actions-url]: https://github.com/sorokya/eolib-c/actions/workflows/build.yml

A core C library for writing applications related to Endless Online

## Features

Read and write the following EO data structures:

- Client packets
- Server packets
- Endless Map Files (EMF)
- Endless Item Files (EIF)
- Endless NPC Files (ENF)
- Endless Spell Files (ESF)
- Endless Class Files (ECF)
- Endless Server Files (Inns, Drops, Shops, etc.)

Utilities:

- Data reader
- Data writer
- Number encoding
- String encoding
- Data encryption
- Packet sequencer

## Installation

### Pre-built (recommended)

Download the release archive for your platform from the
[releases page](https://github.com/sorokya/eolib-c/releases).
Packages are named `eolib-{version}-{platform}.zip` and include both the
C library and the Lua bindings.

**Option 1 — Use directly from the extracted directory (no install needed):**

```cmake
list(APPEND CMAKE_PREFIX_PATH "/path/to/eolib-{version}-{platform}")
find_package(eolib REQUIRED)
target_link_libraries(myapp PRIVATE eolib::eolib)
```

**Option 2 — Copy files to a system prefix manually:**

```bash
# Linux / macOS — copy into /usr/local (or any prefix you prefer)
cp -r eolib-{version}-{platform}/include/* /usr/local/include/
cp -r eolib-{version}-{platform}/lib/*     /usr/local/lib/
cp -r eolib-{version}-{platform}/share/*   /usr/local/share/
```

Then `find_package(eolib REQUIRED)` will find it automatically.

See the [Getting Started](https://sorokya.github.io/eolib-c/getting_started.html)
guide for manual (non-CMake) linking instructions.

### Building from source

#### Requirements

- CMake 3.16+
- C compiler
- libxml2
- json-c

#### Install dependencies

macOS (Homebrew):

```bash
brew install libxml2 json-c
```

Ubuntu/Debian:

```bash
sudo apt-get install -y libxml2-dev libjson-c-dev
```

Windows (MSVC): libxml2 and json-c are handled automatically via the bundled vcpkg configuration.

Windows (MinGW / MSYS2):

```bash
pacman -S --needed mingw-w64-x86_64-libxml2 mingw-w64-x86_64-json-c
```

#### Build

```bash
git clone --recurse-submodules https://github.com/sorokya/eolib-c.git
cd eolib-c

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

#### Run tests

```bash
ctest --test-dir build
```

#### Install

```bash
cmake --install build
```

## WebAssembly (WASM)

A WASM build exists solely to power the
[interactive demo](https://sorokya.github.io/eolib-c/demo/). It is **not
intended for use in real projects**.

If you need eolib in a JavaScript/TypeScript project, use
[eolib-ts](https://github.com/cirras/eolib-ts) instead — it is a proper
TypeScript implementation of the same protocol.

## Lua bindings

Native Lua 5.1+/LuaJIT bindings are available. See
[lua/README.md](lua/README.md) for full setup and API reference.

### Pre-built (recommended)

Download `eolib-{version}-{platform}.zip` from the
[releases page](https://github.com/sorokya/eolib-c/releases) — the main
archive includes both the C library and the Lua bindings.

**Drop-in:** Extract the archive into your project directory.
`require("eolib")` will just work.

### Building from source

Requires Lua development headers in addition to the dependencies above:

- macOS: `brew install lua`
- Ubuntu/Debian: `sudo apt-get install -y liblua5.4-dev lua5.4`
- MinGW/MSYS2: `pacman -S mingw-w64-x86_64-lua`

```bash
cmake -S . -B build -DEOLIB_BUILD_LUA_BINDINGS=ON
cmake --build build --target lua_eolib
```

This produces `build/lua/eolib.so` (or `.dll` on Windows).

### Run Lua tests

```bash
cmake --build build --target lua-test
```

## Protocol Types

All generated structs and packets implement the `EoSerialize` interface (packets
also implement `EoPacket`). Use the `TypeName_init()` function to create a
zero-initialized instance with its vtable set, then use the `eo_*` dispatch
functions to serialize, deserialize, query the size, or free heap memory:

```c
#include "eolib/protocol.h"

// Initialize
LoginRequestClientPacket pkt = LoginRequestClientPacket_init();
pkt.username = "player";
pkt.password = "secret";

// Serialize
size_t needed = eo_get_size((const EoSerialize *)&pkt);
EoWriter writer = eo_writer_init_with_capacity(needed);
eo_serialize((const EoSerialize *)&pkt, &writer);
// writer.data now holds the bytes, writer.length is the byte count

// Free heap fields (e.g. strings, arrays) when done
eo_free((EoSerialize *)&pkt);
eo_writer_free(&writer);

// Deserialize
uint8_t *bytes = ...;
size_t len = ...;
EoReader reader = eo_reader_init(bytes, len);
LoginRequestClientPacket pkt2 = LoginRequestClientPacket_init();
eo_deserialize((EoSerialize *)&pkt2, &reader);
printf("username: %s\n", pkt2.username);
eo_free((EoSerialize *)&pkt2);
```

Packets additionally expose their family/action IDs:

```c
uint8_t family = eo_packet_get_family((const EoPacket *)&pkt);
uint8_t action = eo_packet_get_action((const EoPacket *)&pkt);
```

See the [full documentation](https://sorokya.github.io/eolib-c/) for the complete
API reference, memory safety guide, and worked examples.



All fallible functions return an `EoResult` value. A return value of `EO_SUCCESS` (0)
means the operation completed without error. Any other value indicates a failure and
describes what went wrong.

| Code | Value | Meaning |
|------|-------|---------|
| `EO_SUCCESS` | 0 | Operation completed successfully. |
| `EO_NULL_PTR` | 1 | A required pointer argument was `NULL` (e.g. writer, reader, sequencer, or output buffer). |
| `EO_OVERFLOW` | 2 | An internal size calculation would overflow — growing the writer buffer would exceed `SIZE_MAX`. |
| `EO_ALLOC_FAILED` | 3 | `malloc` or `realloc` returned `NULL`; the system could not satisfy the allocation. |
| `EO_NUMBER_TOO_LARGE` | 4 | A value passed to an EO encode function exceeds the maximum encodable integer (`EO_INT_MAX`). |
| `EO_BUFFER_UNDERRUN` | 5 | The reader does not have enough bytes remaining to satisfy the requested read. |
| `EO_CHUNKED_MODE_DISABLED` | 6 | `eo_reader_next_chunk` was called but chunked reading mode is not enabled on the reader. |
| `EO_INVALID_SEQUENCE_RANGE` | 7 | The derived `seq1` bounds produce an empty range; the start value is outside the representable sequence space. |
| `EO_SEQUENCE_OUT_OF_RANGE` | 8 | A derived sequence byte value falls outside `[0, EO_CHAR_MAX - 1]` and cannot be encoded. |
| `EO_INVALID_DATA` | 9 | Deserialization failed because a sentinel value, file magic string, or required constant field did not match the expected value. |
| `EO_STR_OUT_OF_RANGE` | 10 | A string operation would exceed the allowed length, such as adding a fixed-length string that is too long. |
| `EO_STR_TOO_SHORT` | 11 | A fixed-length string is shorter than expected and padding was not enabled. |

### Example

```c
EoWriter writer = eo_writer_init_with_capacity(16);
EoResult result = eo_writer_add_int(&writer, 12345);
if (result != EO_SUCCESS) {
    fprintf(stderr, "write failed: %d\n", result);
}
free(writer.data);
```

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.
