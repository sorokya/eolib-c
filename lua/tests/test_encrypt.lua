-- test_encrypt.lua: Tests for encryption bindings

local eolib = require("eolib")

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

print("=== Encryption tests ===")

-- server_verification_hash known value (from existing C test)
local hash = eolib.server_verification_hash(0)
assert(type(hash) == "number", "hash is number")
print("  PASS: server_verification_hash returns number")

-- encrypt/decrypt round-trip
local data = "\x01\x02\x03\x04\x05\x06\x07\x08"
local mult = 6
local encrypted = eolib.encrypt_packet(data, mult)
assert_eq(#encrypted, #data, "encrypted length preserved")
local decrypted = eolib.decrypt_packet(encrypted, mult)
assert_bytes_eq(decrypted, data, "encrypt/decrypt round-trip")
print("  PASS: encrypt/decrypt round-trip")

-- generate_swap_multiple range
local m = eolib.generate_swap_multiple()
assert(m >= 6 and m <= 12, "swap multiple in range [6,12]: " .. tostring(m))
print("  PASS: generate_swap_multiple range")

-- swap_multiples
local data2 = "\x06\x0c\x06\x0c\x06"
local swapped = eolib.swap_multiples(data2, 6)
assert_eq(#swapped, #data2, "swap_multiples length preserved")
print("  PASS: swap_multiples returns same length")

-- encrypt does not equal original
local data3 = "\x10\x20\x30\x40\x50\x60"
local enc3 = eolib.encrypt_packet(data3, 6)
local changed = false
for i = 1, #data3 do
    if enc3:byte(i) ~= data3:byte(i) then changed = true break end
end
assert(changed, "encryption should change bytes")
print("  PASS: encrypt changes bytes")

print("=== All encryption tests passed ===")
