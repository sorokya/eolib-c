-- test_data.lua: Tests for EoWriter and EoReader bindings

local eolib = require("eolib")
local ok, err

local function assert_eq(a, b, msg)
    if a ~= b then
        error(string.format("FAIL %s: expected %s, got %s", msg or "?", tostring(b), tostring(a)))
    end
end

local function assert_bytes_eq(a, b, msg)
    if #a ~= #b then
        error(string.format("FAIL %s: length mismatch (%d vs %d)", msg or "?", #a, #b))
    end
    for i = 1, #a do
        if a:byte(i) ~= b:byte(i) then
            error(string.format("FAIL %s: byte %d differs (%d vs %d)", msg or "?", i, a:byte(i), b:byte(i)))
        end
    end
end

print("=== EoWriter / EoReader tests ===")

-- Round-trip numbers
local w = eolib.EoWriter.new()
w:add_char(42)
w:add_short(1234)
w:add_three(123456)
w:add_int(1000000)
local bytes = w:to_bytes()
assert_eq(#bytes, 1 + 2 + 3 + 4, "write length")

local r = eolib.EoReader.new(bytes)
assert_eq(r:get_char(), 42, "get_char")
assert_eq(r:get_short(), 1234, "get_short")
assert_eq(r:get_three(), 123456, "get_three")
assert_eq(r:get_int(), 1000000, "get_int")
print("  PASS: number round-trip")

-- Byte
local w2 = eolib.EoWriter.new()
w2:add_byte(0xFF)
local r2 = eolib.EoReader.new(w2:to_bytes())
assert_eq(r2:get_byte(), 0xFF, "byte 0xFF")
print("  PASS: byte")

-- String
local w3 = eolib.EoWriter.new()
w3:add_string("hello")
local r3 = eolib.EoReader.new(w3:to_bytes())
assert_eq(r3:get_string(), "hello", "string round-trip")
print("  PASS: string")

-- Encoded string
local w4 = eolib.EoWriter.new()
w4:add_encoded_string("hello")
local r4 = eolib.EoReader.new(w4:to_bytes())
assert_eq(r4:get_encoded_string(), "hello", "encoded_string round-trip")
print("  PASS: encoded_string")

-- Fixed string
local w5 = eolib.EoWriter.new()
w5:add_fixed_string("hi", 5, true)
assert_eq(w5:get_length(), 5, "fixed_string padded length")
print("  PASS: fixed_string padded")

-- add_bytes / get_bytes
local w6 = eolib.EoWriter.new()
local raw = "\x01\x02\x03\x04"
w6:add_bytes(raw)
local r6 = eolib.EoReader.new(w6:to_bytes())
assert_bytes_eq(r6:get_bytes(4), raw, "add_bytes/get_bytes")
print("  PASS: raw bytes")

-- Chunked mode
local w7 = eolib.EoWriter.new()
w7:add_char(1)
w7:add_byte(0xFF) -- break byte
w7:add_char(2)
local r7 = eolib.EoReader.new(w7:to_bytes())
r7:set_chunked_reading_mode(true)
assert_eq(r7:get_char(), 1, "chunked first char")
r7:next_chunk()
assert_eq(r7:get_char(), 2, "chunked second char")
print("  PASS: chunked reading")

-- remaining
local w8 = eolib.EoWriter.new()
w8:add_char(10)
w8:add_char(20)
local r8 = eolib.EoReader.new(w8:to_bytes())
assert_eq(r8:remaining(), 2, "remaining before read")
r8:get_char()
assert_eq(r8:remaining(), 1, "remaining after read")
print("  PASS: remaining")

-- encode/decode number
local encoded = eolib.encode_number(12345)
assert_eq(#encoded, 4, "encode_number length")
assert_eq(eolib.decode_number(encoded), 12345, "decode_number roundtrip")
print("  PASS: encode/decode number")

-- encode/decode string
local s = "hello"
local enc = eolib.encode_string(s)
assert_eq(eolib.decode_string(enc), s, "encode/decode string roundtrip")
print("  PASS: encode/decode string")

-- constants
assert(eolib.CHAR_MAX == 253, "CHAR_MAX")
assert(eolib.SHORT_MAX == 64009, "SHORT_MAX")
assert(eolib.BREAK_BYTE == 0xFF, "BREAK_BYTE")
print("  PASS: constants")

-- string_sanitization_mode
local w9 = eolib.EoWriter.new()
assert_eq(w9:get_string_sanitization_mode(), false, "default sanitization mode")
w9:set_string_sanitization_mode(true)
assert_eq(w9:get_string_sanitization_mode(), true, "enabled sanitization mode")
print("  PASS: string sanitization mode")

print("=== All data tests passed ===")
