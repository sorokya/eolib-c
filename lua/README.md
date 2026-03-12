# eolib Lua Bindings

Native C extension module exposing the full eolib API to Lua 5.1, 5.2, 5.3, 5.4,
and LuaJIT.

## Building

The Lua bindings are built as part of the main eolib CMake project when the
`EOLIB_BUILD_LUA_BINDINGS=ON` option is enabled. Lua development headers must
be installed first.

macOS (Homebrew):

```sh
brew install lua
```

Debian/Ubuntu:

```sh
sudo apt-get install -y liblua5.4-dev
```

Then build:

```sh
cmake . -DEOLIB_BUILD_LUA_BINDINGS=ON
make lua_eolib
```

This produces `lua/eolib.so` (or `eolib.dll` on Windows).

## Installation

```sh
make install
```

The shared library (`eolib.so` / `eolib.dll`) is installed to
`${CMAKE_INSTALL_LIBDIR}`. Add its parent directory to `LUA_CPATH` so Lua can find
it.

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
make lua-test
```

Or manually from a Lua interpreter with `eolib.so` in `LUA_CPATH`:

```sh
LUA_CPATH="./lua/?.so" lua lua/tests/test_data.lua
LUA_CPATH="./lua/?.so" lua lua/tests/test_encrypt.lua
LUA_CPATH="./lua/?.so" lua lua/tests/test_sequencer.lua
LUA_CPATH="./lua/?.so" lua lua/tests/test_protocol.lua
```

## IDE support (lua-language-server)

A LuaCATS annotation file `lua/eolib.d.lua` is generated alongside the source
code. It provides full type information — classes, fields, enums, and function
signatures — for every type in the library.

Add it to your project's `.luarc.json`:

```json
{
  "workspace.library": ["/path/to/eolib-c/lua/eolib.d.lua"]
}
```

Or download `eolib.d.lua` from the [GitHub release](https://github.com/sorokya/eolib-c/releases) and drop it anywhere in your workspace library path.

Once configured, editors using [lua-language-server](https://github.com/LuaLS/lua-language-server) (VS Code, Neovim, etc.) will show:
- Autocompletion for all packet types, enums, structs, and module functions
- Inline type hints and parameter docs
- Type-error highlighting

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
