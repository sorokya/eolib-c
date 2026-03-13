#include "eolib_lua.h"
#include "eolib/time.h"

static int lua_eo_time(lua_State *L)
{
    lua_pushinteger(L, (lua_Integer)eo_time());
    return 1;
}

void lua_time_register(lua_State *L, int module_idx)
{
    lua_pushcfunction(L, lua_eo_time);
    lua_setfield(L, module_idx, "time");
}
