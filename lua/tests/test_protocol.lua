-- test_protocol.lua: Tests for protocol type bindings

local eolib = require("eolib")

local function assert_eq(a, b, msg)
    if a ~= b then
        error(string.format("FAIL %s: expected %s, got %s", msg or "?", tostring(b), tostring(a)))
    end
end

print("=== Protocol tests ===")

-- Enum tables should exist and have integer values
assert(type(eolib.AdminLevel) == "table", "AdminLevel is a table")
assert(type(eolib.AdminLevel.Player) == "number", "AdminLevel.Player is a number")
assert(eolib.AdminLevel.Player == 0, "AdminLevel.Player == 0")
assert(type(eolib.PacketFamily) == "table", "PacketFamily is a table")
assert(type(eolib.PacketAction) == "table", "PacketAction is a table")
print("  PASS: enum tables and values")

-- Packet types should be accessible
assert(type(eolib.AccountCreateClientPacket) == "table",
    "AccountCreateClientPacket is a table")
assert(type(eolib.AccountCreateClientPacket.new) == "function",
    "AccountCreateClientPacket.new is a function")
assert(type(eolib.AccountCreateClientPacket.deserialize) == "function",
    "AccountCreateClientPacket.deserialize is a function")
print("  PASS: packet type table structure")

-- new() returns a table with metatable methods
local pkt = eolib.AccountCreateClientPacket.new()
assert(type(pkt) == "table", "new() returns table")
assert(type(pkt.serialize) == "function", "instance has serialize method")
assert(type(pkt.write) == "function", "instance has write method")
print("  PASS: instance methods available")

-- Direction enum
assert(type(eolib.Direction) == "table", "Direction is a table")
assert(type(eolib.Direction.Down) == "number", "Direction.Down is number")
print("  PASS: Direction enum")

-- Coords struct
assert(type(eolib.Coords) == "table", "Coords is a table")
local c = eolib.Coords.new()
assert(type(c) == "table", "Coords.new() returns table")
c.x = 10
c.y = 20
local bytes = c:serialize()
assert(type(bytes) == "string", "Coords:serialize() returns string")
assert(#bytes > 0, "Coords:serialize() returns non-empty bytes")
print("  PASS: Coords serialize")

-- Coords deserialize round-trip
local c2 = eolib.Coords.deserialize(bytes)
assert(type(c2) == "table", "Coords.deserialize() returns table")
assert_eq(c2.x, 10, "Coords.x round-trip")
assert_eq(c2.y, 20, "Coords.y round-trip")
print("  PASS: Coords round-trip")

-- write() method
local w = eolib.EoWriter.new()
c:write(w)
local written_bytes = w:to_bytes()
assert_eq(#written_bytes, #bytes, "write() produces same length as serialize()")
print("  PASS: write() method")

print("=== All protocol tests passed ===")
