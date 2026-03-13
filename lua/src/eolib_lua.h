#ifndef EOLIB_LUA_H
#define EOLIB_LUA_H

#include <lua.h>
#include <lauxlib.h>
#include "eolib/data.h"

typedef struct { EoWriter writer; } LuaEoWriter;

#define EOLIB_WRITER_MT    "eolib.EoWriter"
#define EOLIB_READER_MT    "eolib.EoReader"
#define EOLIB_SEQUENCER_MT "eolib.EoSequencer"

/* Lua 5.1 compatibility shims */
#if LUA_VERSION_NUM < 502
#define lua_rawlen(L, i)    ((int)lua_objlen((L), (i)))

static inline void eolib_setmetatable(lua_State *L, const char *name)
{
    luaL_getmetatable(L, name);
    lua_setmetatable(L, -2);
}
#else
static inline void eolib_setmetatable(lua_State *L, const char *name)
{
    luaL_setmetatable(L, name);
}
#endif

/* Sub-module registration functions */
void lua_data_register(lua_State *L, int module_idx);
void lua_encrypt_register(lua_State *L, int module_idx);
void lua_rng_register(lua_State *L, int module_idx);
void lua_sequencer_register(lua_State *L, int module_idx);
void lua_protocol_register(lua_State *L, int module_idx);

#endif /* EOLIB_LUA_H */
