#include "eolib_lua.h"
#include "sequencer.h"
#include "result.h"

/* -------------------------------------------------------------------------
 * EoSequencer userdata
 * ---------------------------------------------------------------------- */

typedef struct
{
    EoSequencer sequencer;
} LuaEoSequencer;

static LuaEoSequencer *check_sequencer(lua_State *L, int idx)
{
    return (LuaEoSequencer *)luaL_checkudata(L, idx, EOLIB_SEQUENCER_MT);
}

static int sequencer_new(lua_State *L)
{
    int32_t start = (int32_t)luaL_checkinteger(L, 1);
    LuaEoSequencer *ud = (LuaEoSequencer *)lua_newuserdata(L, sizeof(LuaEoSequencer));
    ud->sequencer = eo_sequencer_init(start);
    eolib_setmetatable(L, EOLIB_SEQUENCER_MT);
    return 1;
}

static int sequencer_gc(lua_State *L)
{
    (void)L;
    return 0;
}

static int sequencer_next(lua_State *L)
{
    LuaEoSequencer *ud = check_sequencer(L, 1);
    int32_t val = 0;
    EoResult r = eo_sequencer_next(&ud->sequencer, &val);
    if (r != EO_SUCCESS)
        return luaL_error(L, "sequencer next failed: %s", eo_result_string(r));
    lua_pushinteger(L, (lua_Integer)val);
    return 1;
}

static const luaL_Reg sequencer_methods[] = {
    {"next", sequencer_next},
    {NULL, NULL}
};

/* -------------------------------------------------------------------------
 * Standalone sequence utilities
 * ---------------------------------------------------------------------- */

static int lua_generate_sequence_start(lua_State *L)
{
    lua_pushinteger(L, (lua_Integer)eo_generate_sequence_start());
    return 1;
}

static int lua_sequence_start_from_init(lua_State *L)
{
    int32_t s1 = (int32_t)luaL_checkinteger(L, 1);
    int32_t s2 = (int32_t)luaL_checkinteger(L, 2);
    lua_pushinteger(L, (lua_Integer)eo_sequence_start_from_init(s1, s2));
    return 1;
}

static int lua_sequence_start_from_ping(lua_State *L)
{
    int32_t s1 = (int32_t)luaL_checkinteger(L, 1);
    int32_t s2 = (int32_t)luaL_checkinteger(L, 2);
    lua_pushinteger(L, (lua_Integer)eo_sequence_start_from_ping(s1, s2));
    return 1;
}

static int lua_sequence_init_bytes(lua_State *L)
{
    int32_t start = (int32_t)luaL_checkinteger(L, 1);
    uint8_t out[2] = {0};
    EoResult r = eo_sequence_init_bytes(start, out);
    if (r != EO_SUCCESS)
        return luaL_error(L, "sequence_init_bytes failed: %s", eo_result_string(r));
    lua_pushinteger(L, (lua_Integer)out[0]);
    lua_pushinteger(L, (lua_Integer)out[1]);
    return 2;
}

static int lua_sequence_ping_bytes(lua_State *L)
{
    int32_t start = (int32_t)luaL_checkinteger(L, 1);
    uint8_t out[2] = {0};
    EoResult r = eo_sequence_ping_bytes(start, out);
    if (r != EO_SUCCESS)
        return luaL_error(L, "sequence_ping_bytes failed: %s", eo_result_string(r));
    lua_pushinteger(L, (lua_Integer)out[0]);
    lua_pushinteger(L, (lua_Integer)out[1]);
    return 2;
}

/* -------------------------------------------------------------------------
 * Registration
 * ---------------------------------------------------------------------- */

void lua_sequencer_register(lua_State *L, int module_idx)
{
    luaL_newmetatable(L, EOLIB_SEQUENCER_MT);

    lua_newtable(L);
#if LUA_VERSION_NUM < 502
    luaL_register(L, NULL, sequencer_methods);
#else
    luaL_setfuncs(L, sequencer_methods, 0);
#endif
    lua_setfield(L, -2, "__index");

    lua_pushcfunction(L, sequencer_gc);
    lua_setfield(L, -2, "__gc");

    lua_pop(L, 1);

    /* EoSequencer table */
    lua_newtable(L);
    lua_pushcfunction(L, sequencer_new);
    lua_setfield(L, -2, "new");
    lua_setfield(L, module_idx, "EoSequencer");

    /* Standalone utilities */
    lua_pushcfunction(L, lua_generate_sequence_start);
    lua_setfield(L, module_idx, "generate_sequence_start");

    lua_pushcfunction(L, lua_sequence_start_from_init);
    lua_setfield(L, module_idx, "sequence_start_from_init");

    lua_pushcfunction(L, lua_sequence_start_from_ping);
    lua_setfield(L, module_idx, "sequence_start_from_ping");

    lua_pushcfunction(L, lua_sequence_init_bytes);
    lua_setfield(L, module_idx, "sequence_init_bytes");

    lua_pushcfunction(L, lua_sequence_ping_bytes);
    lua_setfield(L, module_idx, "sequence_ping_bytes");
}
