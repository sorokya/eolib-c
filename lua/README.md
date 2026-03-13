# eolib Lua Bindings

Native C extension module exposing the full eolib API to Lua 5.1, 5.2, 5.3, 5.4,
and LuaJIT.

## Installation & setup

### Pre-built releases (recommended)

Download `eolib-{version}-{platform}.zip` from the
[releases page](https://github.com/sorokya/eolib-c/releases) — the main
archive includes both the C library and the Lua bindings.

**Option 1 — Drop-in (quickest):** Extract the archive into your project
directory. Lua searches `./` by default so `require("eolib")` will just work.

**Option 2 — Copy to a system prefix:**

```sh
# Linux / macOS
cp eolib-{version}-{platform}/lib/lua/5.4/eolib.so      /usr/local/lib/lua/5.4/
cp eolib-{version}-{platform}/lib/libeolib.*             /usr/local/lib/
cp eolib-{version}-{platform}/share/lua/5.4/eolib.d.lua /usr/local/share/lua/5.4/
```

### Building from source

Requires a C toolchain, CMake 3.16+, libxml2, json-c, and Lua headers.

macOS:

```sh
xcode-select --install && brew install cmake libxml2 json-c lua
```

Linux (Ubuntu / Debian):

```sh
sudo apt-get install gcc cmake build-essential libxml2-dev libjson-c-dev liblua5.4-dev lua5.4
```

Windows — MSVC: Install [Visual Studio 2022 Community](https://visualstudio.microsoft.com/vs/community/)
with the **Desktop development with C++** workload. libxml2 and json-c are handled automatically via the bundled vcpkg configuration.

Windows — MinGW via MSYS2:

```sh
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-libxml2 mingw-w64-x86_64-json-c mingw-w64-x86_64-lua
```

Then clone and build:

```sh
git clone --recurse-submodules https://github.com/sorokya/eolib-c.git
cd eolib-c

cmake -S . -B build -DEOLIB_BUILD_LUA_BINDINGS=ON
cmake --build build
cmake --install build
```

### Troubleshooting: `module 'eolib' not found`

Some systems (e.g. macOS with Homebrew Lua) use a different prefix than
`/usr/local`. Check where your Lua looks:

```sh
lua -e "print(package.cpath)"
```

Then copy files there, or set `LUA_CPATH` manually:

```sh
export LUA_CPATH="/usr/local/lib/lua/5.4/?.so;;"
```

### IDE support (lua-language-server)

`eolib.d.lua` provides full type annotations for
[lua-language-server](https://github.com/LuaLS/lua-language-server). For the
drop-in method, having it in your project root is enough. For a system install,
luals typically picks it up from `share/lua/5.4/` automatically.

## Quick start

```lua
local eolib = require("eolib")
```

---

## API Reference

### EoWriter

```lua
local w = eolib.EoWriter.new()

w:add_byte(0xFF)
w:add_char(42)           -- 1-byte EO number
w:add_short(1234)        -- 2-byte EO number
w:add_three(123456)      -- 3-byte EO number
w:add_int(1000000)       -- 4-byte EO number
w:add_string("hello")
w:add_encoded_string("hello")
w:add_fixed_string("hi", 5, true)          -- length, padded
w:add_fixed_encoded_string("hi", 5, false)
w:add_bytes("\x01\x02\x03")               -- raw bytes as Lua string

local bytes = w:to_bytes()                -- returns Lua string
local n     = w:get_length()

w:set_string_sanitization_mode(true)
local mode = w:get_string_sanitization_mode()  -- bool
```

### EoReader

```lua
local r = eolib.EoReader.new(bytes)  -- bytes is a Lua string

local b  = r:get_byte()
local c  = r:get_char()
local s  = r:get_short()
local t  = r:get_three()
local i  = r:get_int()
local st = r:get_string()
local es = r:get_encoded_string()
local fs = r:get_fixed_string(5)
local fe = r:get_fixed_encoded_string(5)
local raw = r:get_bytes(4)           -- returns Lua string

local rem = r:remaining()

r:set_chunked_reading_mode(true)
local chunked = r:get_chunked_reading_mode()
r:next_chunk()
```

### Encryption

```lua
-- Server challenge-response hash
local hash = eolib.server_verification_hash(challenge)

-- Encrypt / decrypt a packet in-place (returns new string)
local encrypted = eolib.encrypt_packet(bytes, swap_multiple)
local decrypted = eolib.decrypt_packet(bytes, swap_multiple)

-- Generate a cryptographically random swap multiple (6–12)
local mult = eolib.generate_swap_multiple()

-- Reverse sequences of bytes divisible by a multiple
local swapped = eolib.swap_multiples(bytes, multiple)
```

### Sequencer

```lua
local seq = eolib.EoSequencer.new(start)
local next_val = seq:next()

-- Generate a random start value
local start = eolib.generate_sequence_start()

-- Recover start value from server handshake bytes
local start = eolib.sequence_start_from_init(s1, s2)
local start = eolib.sequence_start_from_ping(s1, s2)

-- Get the two handshake bytes from a start value
local b1, b2 = eolib.sequence_init_bytes(start)
local b1, b2 = eolib.sequence_ping_bytes(start)
```

### Low-level encoding utilities

```lua
local encoded = eolib.encode_number(12345)    -- returns 4-byte string
local value   = eolib.decode_number(encoded)  -- returns integer

local enc = eolib.encode_string(s)  -- in-place transform, returns string
local dec = eolib.decode_string(s)

local win1252 = eolib.string_to_windows_1252(utf8_str)
```

### Constants

```lua
eolib.CHAR_MAX        -- 253
eolib.SHORT_MAX       -- 64009
eolib.THREE_MAX       -- 16194277
eolib.INT_MAX         -- 4097152081
eolib.NUMBER_PADDING  -- 0xFE
eolib.BREAK_BYTE      -- 0xFF
```

### Enums

Every protocol enum is exposed as a nested table of integer constants:

```lua
eolib.AdminLevel.Player    -- 0
eolib.AdminLevel.Guide     -- 1
eolib.AdminLevel.GM        -- 2

eolib.Direction.Down  -- 0
eolib.Direction.Left  -- 1
eolib.Direction.Up    -- 2
eolib.Direction.Right -- 3

eolib.PacketFamily.Connection  -- integer
eolib.PacketAction.Ping        -- integer
-- … all 54 enums
```

### Protocol types (packets and structs)

Every protocol type is exposed as a table with `new()` and `deserialize()` static
functions, plus `serialize()` and `write()` instance methods:

```lua
-- Create an empty instance
local pkt = eolib.LoginRequestClientPacket.new()

-- Populate fields
pkt.username = "player"
pkt.password = "secret"

-- Serialize to bytes (Lua string)
local bytes = pkt:serialize()

-- Write into an existing EoWriter
local w = eolib.EoWriter.new()
pkt:write(w)

-- Deserialize from bytes
local pkt2 = eolib.LoginRequestClientPacket.deserialize(bytes)
print(pkt2.username)  -- "player"
```

#### Nested structs

Nested struct fields are represented as Lua tables:

```lua
local pkt = eolib.WelcomeReplyServerPacket.new()
pkt.character_name = "hero"
pkt.character_id   = 42
-- etc.
```

#### Array fields

Array fields are Lua arrays (1-indexed):

```lua
local pkt = eolib.NpcPlayerServerPacket.new()
pkt.npc_kills = { 1, 2, 3 }
```

#### Switch / union fields

Switch fields include a `{field}_data` table for the active case:

```lua
local item = eolib.ItemDef.new()
item.type      = eolib.ItemType.Armor
item.type_data = { defense = 50, weight = 20 }
```

## Running the tests

```sh
cmake --build build --target lua-test
```

Or manually from a Lua interpreter with `eolib.so` in `LUA_CPATH`:

```sh
LUA_CPATH="./build/lua/?.so" lua lua/tests/test_data.lua
LUA_CPATH="./build/lua/?.so" lua lua/tests/test_encrypt.lua
LUA_CPATH="./build/lua/?.so" lua lua/tests/test_sequencer.lua
LUA_CPATH="./build/lua/?.so" lua lua/tests/test_protocol.lua
```

## Error handling

All functions raise Lua errors on failure (e.g. buffer underrun, allocation failure).
Use `pcall` for graceful error handling:

```lua
local ok, err = pcall(function()
    local r = eolib.EoReader.new("")
    r:get_int()  -- will error: buffer underrun
end)
if not ok then print("caught: " .. err) end
```
