#include "eolib_lua.h"

int luaopen_eolib(lua_State *L);

int luaopen_eolib(lua_State *L)
{
    lua_newtable(L);
    int module_idx = lua_gettop(L);

    lua_data_register(L, module_idx);
    lua_encrypt_register(L, module_idx);
    lua_sequencer_register(L, module_idx);
    lua_protocol_register(L, module_idx);

    return 1;
}
