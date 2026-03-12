-- test_sequencer.lua: Tests for sequencer bindings

local eolib = require("eolib")

local function assert_eq(a, b, msg)
    if a ~= b then
        error(string.format("FAIL %s: expected %s, got %s", msg or "?", tostring(b), tostring(a)))
    end
end

print("=== Sequencer tests ===")

-- Basic next
local seq = eolib.EoSequencer.new(10)
local n1 = seq:next()
assert(type(n1) == "number", "next returns number")
print("  PASS: next returns number")

-- Wraparound: sequencer counter should wrap at 10 (0..9)
local seq2 = eolib.EoSequencer.new(0)
local vals = {}
for i = 1, 12 do vals[i] = seq2:next() end
-- Check that values wrap around (at some point a value repeats the cycle)
assert(vals[11] == vals[1], "counter wraps at 10: " .. tostring(vals[1]) .. " vs " .. tostring(vals[11]))
assert(vals[12] == vals[2], "counter wraps at 10 second")
print("  PASS: counter wraparound")

-- generate_sequence_start
local start = eolib.generate_sequence_start()
assert(type(start) == "number", "generate_sequence_start returns number")
print("  PASS: generate_sequence_start")

-- sequence_start_from_init / ping
local s = eolib.sequence_start_from_init(2, 3)
assert(type(s) == "number", "sequence_start_from_init returns number")
local s2 = eolib.sequence_start_from_ping(5, 7)
assert(type(s2) == "number", "sequence_start_from_ping returns number")
print("  PASS: sequence_start_from_init/ping")

-- sequence_init_bytes / ping_bytes round-trip
local b1, b2 = eolib.sequence_init_bytes(start)
assert(type(b1) == "number" and type(b2) == "number", "init_bytes returns two numbers")
local recovered = eolib.sequence_start_from_init(b1, b2)
assert_eq(recovered, start, "init_bytes round-trip")
print("  PASS: sequence_init_bytes round-trip")

local p1, p2 = eolib.sequence_ping_bytes(start)
assert(type(p1) == "number" and type(p2) == "number", "ping_bytes returns two numbers")
local recovered2 = eolib.sequence_start_from_ping(p1, p2)
assert_eq(recovered2, start, "ping_bytes round-trip")
print("  PASS: sequence_ping_bytes round-trip")

print("=== All sequencer tests passed ===")
