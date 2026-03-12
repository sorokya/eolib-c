#include "eolib_lua.h"
#include "encrypt.h"

#include <stdlib.h>
#include <string.h>

static int lua_server_verification_hash(lua_State *L)
{
    int32_t challenge = (int32_t)luaL_checkinteger(L, 1);
    lua_pushinteger(L, (lua_Integer)eo_server_verification_hash(challenge));
    return 1;
}

static int lua_swap_multiples(lua_State *L)
{
    size_t len;
    const char *data = luaL_checklstring(L, 1, &len);
    uint8_t multiple = (uint8_t)luaL_checkinteger(L, 2);

    uint8_t *buf = (uint8_t *)malloc(len);
    if (!buf)
        return luaL_error(L, "allocation failed");
    memcpy(buf, data, len);
    eo_swap_multiples(buf, len, multiple);
    lua_pushlstring(L, (const char *)buf, len);
    free(buf);
    return 1;
}

static int lua_generate_swap_multiple(lua_State *L)
{
    lua_pushinteger(L, (lua_Integer)eo_generate_swap_multiple());
    return 1;
}

static int lua_encrypt_packet(lua_State *L)
{
    size_t len;
    const char *data = luaL_checklstring(L, 1, &len);
    uint8_t swap_multiple = (uint8_t)luaL_checkinteger(L, 2);

    uint8_t *buf = (uint8_t *)malloc(len);
    if (!buf)
        return luaL_error(L, "allocation failed");
    memcpy(buf, data, len);
    eo_encrypt_packet(buf, len, swap_multiple);
    lua_pushlstring(L, (const char *)buf, len);
    free(buf);
    return 1;
}

static int lua_decrypt_packet(lua_State *L)
{
    size_t len;
    const char *data = luaL_checklstring(L, 1, &len);
    uint8_t swap_multiple = (uint8_t)luaL_checkinteger(L, 2);

    uint8_t *buf = (uint8_t *)malloc(len);
    if (!buf)
        return luaL_error(L, "allocation failed");
    memcpy(buf, data, len);
    eo_decrypt_packet(buf, len, swap_multiple);
    lua_pushlstring(L, (const char *)buf, len);
    free(buf);
    return 1;
}

void lua_encrypt_register(lua_State *L, int module_idx)
{
    lua_pushcfunction(L, lua_server_verification_hash);
    lua_setfield(L, module_idx, "server_verification_hash");

    lua_pushcfunction(L, lua_swap_multiples);
    lua_setfield(L, module_idx, "swap_multiples");

    lua_pushcfunction(L, lua_generate_swap_multiple);
    lua_setfield(L, module_idx, "generate_swap_multiple");

    lua_pushcfunction(L, lua_encrypt_packet);
    lua_setfield(L, module_idx, "encrypt_packet");

    lua_pushcfunction(L, lua_decrypt_packet);
    lua_setfield(L, module_idx, "decrypt_packet");
}
