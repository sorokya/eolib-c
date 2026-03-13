-- test_rng.lua: Tests for RNG bindings

local eolib = require("eolib")

local function assert_eq(a, b, msg)
    if a ~= b then
        error(string.format("FAIL %s: expected %s, got %s", msg or "?", tostring(b), tostring(a)))
    end
end

print("=== RNG tests ===")

-- srand/rand are functions
assert(type(eolib.srand) == "function", "srand is function")
assert(type(eolib.rand) == "function", "rand is function")
assert(type(eolib.rand_range) == "function", "rand_range is function")
print("  PASS: rng functions exist")

-- same seed produces same sequence
eolib.srand(12345)
local a0 = eolib.rand()
local a1 = eolib.rand()
local a2 = eolib.rand()

eolib.srand(12345)
local b0 = eolib.rand()
local b1 = eolib.rand()
local b2 = eolib.rand()

assert_eq(a0, b0, "deterministic rand [0]")
assert_eq(a1, b1, "deterministic rand [1]")
assert_eq(a2, b2, "deterministic rand [2]")
print("  PASS: same seed produces same sequence")

-- rand returns 31-bit positive integer
eolib.srand(1)
for i = 1, 20 do
    local v = eolib.rand()
    assert(type(v) == "number" and v >= 0, "rand returns non-negative integer: " .. tostring(v))
    assert(v < 2^31, "rand fits in 31 bits: " .. tostring(v))
end
print("  PASS: rand returns 31-bit positive integers")

-- rand_range stays within bounds
eolib.srand(42)
for i = 1, 500 do
    local v = eolib.rand_range(10, 20)
    assert(v >= 10 and v <= 20, "rand_range [10,20] out of bounds: " .. tostring(v))
end
print("  PASS: rand_range stays within bounds")

-- rand_range with single value always returns that value
eolib.srand(7)
for i = 1, 10 do
    assert_eq(eolib.rand_range(5, 5), 5, "rand_range single value")
end
print("  PASS: rand_range single value")

-- different seeds produce different sequences (probabilistic sanity check)
eolib.srand(1)
local seq1 = {eolib.rand(), eolib.rand(), eolib.rand()}
eolib.srand(2)
local seq2 = {eolib.rand(), eolib.rand(), eolib.rand()}
local all_same = seq1[1] == seq2[1] and seq1[2] == seq2[2] and seq1[3] == seq2[3]
assert(not all_same, "different seeds should produce different sequences")
print("  PASS: different seeds produce different sequences")

print("=== All RNG tests passed ===")
