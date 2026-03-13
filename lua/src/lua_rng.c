#include "eolib_lua.h"
#include "eolib/rng.h"

static int lua_eo_srand(lua_State *L)
{
    uint32_t seed = (uint32_t)luaL_checkinteger(L, 1);
    eo_srand(seed);
    return 0;
}

static int lua_eo_rand(lua_State *L)
{
    lua_pushinteger(L, (lua_Integer)eo_rand());
    return 1;
}

static int lua_eo_rand_range(lua_State *L)
{
    uint32_t min = (uint32_t)luaL_checkinteger(L, 1);
    uint32_t max = (uint32_t)luaL_checkinteger(L, 2);
    lua_pushinteger(L, (lua_Integer)eo_rand_range(min, max));
    return 1;
}

void lua_rng_register(lua_State *L, int module_idx)
{
    lua_pushcfunction(L, lua_eo_srand);
    lua_setfield(L, module_idx, "srand");

    lua_pushcfunction(L, lua_eo_rand);
    lua_setfield(L, module_idx, "rand");

    lua_pushcfunction(L, lua_eo_rand_range);
    lua_setfield(L, module_idx, "rand_range");
}
