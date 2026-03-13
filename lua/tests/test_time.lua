-- test_time.lua: Tests for time binding

local eolib = require("eolib")

print("=== Time tests ===")

-- time is a function
assert(type(eolib.time) == "function", "time is function")
print("  PASS: time function exists")

-- time() returns a positive integer
local t = eolib.time()
assert(type(t) == "number", "time() returns number")
assert(t > 0, "time() returns positive value: " .. tostring(t))
-- Borland epoch floor: the formula computes from the 1970 epoch + 18000 offset,
-- so values for 2025+ should be above ~1740000000.
assert(t > 1740000000, "time() is above year-2025 floor: " .. tostring(t))
print("  PASS: time() returns positive Borland-epoch seconds")

-- two consecutive calls return the same or slightly later value
local t1 = eolib.time()
local t2 = eolib.time()
assert(t2 >= t1, "time() is monotonic: t2=" .. tostring(t2) .. " t1=" .. tostring(t1))
assert(t2 < t1 + 2, "time() does not jump unexpectedly: t1=" .. tostring(t1) .. " t2=" .. tostring(t2))
print("  PASS: time() is monotonic within 2-second window")

-- seeding rng with eo_time() produces a valid rand sequence
eolib.srand(eolib.time())
local v = eolib.rand()
assert(type(v) == "number" and v >= 0 and v < 2^31, "rand after srand(time()) is valid: " .. tostring(v))
print("  PASS: srand(time()) produces valid rand output")

print("=== All time tests passed ===")
