#include "eolib_lua.h"
#include "eolib/data.h"
#include "eolib/result.h"

#include <stdlib.h>
#include <string.h>

/* -------------------------------------------------------------------------
 * EoWriter userdata
 * ---------------------------------------------------------------------- */

static LuaEoWriter *check_writer(lua_State *L, int idx)
{
    return (LuaEoWriter *)luaL_checkudata(L, idx, EOLIB_WRITER_MT);
}

static int writer_new(lua_State *L)
{
    LuaEoWriter *ud = (LuaEoWriter *)lua_newuserdata(L, sizeof(LuaEoWriter));
    ud->writer = eo_writer_init();
    eolib_setmetatable(L, EOLIB_WRITER_MT);
    return 1;
}

static int writer_gc(lua_State *L)
{
    LuaEoWriter *ud = (LuaEoWriter *)luaL_checkudata(L, 1, EOLIB_WRITER_MT);
    eo_writer_free(&ud->writer);
    return 0;
}

static int writer_add_byte(lua_State *L)
{
    LuaEoWriter *ud = check_writer(L, 1);
    uint8_t val = (uint8_t)luaL_checkinteger(L, 2);
    EoResult r = eo_writer_add_byte(&ud->writer, val);
    if (r != EO_SUCCESS)
        return luaL_error(L, "add_byte failed: %s", eo_result_string(r));
    return 0;
}

static int writer_add_char(lua_State *L)
{
    LuaEoWriter *ud = check_writer(L, 1);
    int32_t val = (int32_t)luaL_checkinteger(L, 2);
    EoResult r = eo_writer_add_char(&ud->writer, val);
    if (r != EO_SUCCESS)
        return luaL_error(L, "add_char failed: %s", eo_result_string(r));
    return 0;
}

static int writer_add_short(lua_State *L)
{
    LuaEoWriter *ud = check_writer(L, 1);
    int32_t val = (int32_t)luaL_checkinteger(L, 2);
    EoResult r = eo_writer_add_short(&ud->writer, val);
    if (r != EO_SUCCESS)
        return luaL_error(L, "add_short failed: %s", eo_result_string(r));
    return 0;
}

static int writer_add_three(lua_State *L)
{
    LuaEoWriter *ud = check_writer(L, 1);
    int32_t val = (int32_t)luaL_checkinteger(L, 2);
    EoResult r = eo_writer_add_three(&ud->writer, val);
    if (r != EO_SUCCESS)
        return luaL_error(L, "add_three failed: %s", eo_result_string(r));
    return 0;
}

static int writer_add_int(lua_State *L)
{
    LuaEoWriter *ud = check_writer(L, 1);
    int32_t val = (int32_t)luaL_checkinteger(L, 2);
    EoResult r = eo_writer_add_int(&ud->writer, val);
    if (r != EO_SUCCESS)
        return luaL_error(L, "add_int failed: %s", eo_result_string(r));
    return 0;
}

static int writer_add_string(lua_State *L)
{
    LuaEoWriter *ud = check_writer(L, 1);
    const char *s = luaL_checkstring(L, 2);
    EoResult r = eo_writer_add_string(&ud->writer, s);
    if (r != EO_SUCCESS)
        return luaL_error(L, "add_string failed: %s", eo_result_string(r));
    return 0;
}

static int writer_add_encoded_string(lua_State *L)
{
    LuaEoWriter *ud = check_writer(L, 1);
    const char *s = luaL_checkstring(L, 2);
    EoResult r = eo_writer_add_encoded_string(&ud->writer, s);
    if (r != EO_SUCCESS)
        return luaL_error(L, "add_encoded_string failed: %s", eo_result_string(r));
    return 0;
}

static int writer_add_fixed_string(lua_State *L)
{
    LuaEoWriter *ud = check_writer(L, 1);
    const char *s = luaL_checkstring(L, 2);
    size_t length = (size_t)luaL_checkinteger(L, 3);
    int padded = lua_toboolean(L, 4);
    EoResult r = eo_writer_add_fixed_string(&ud->writer, s, length, padded ? true : false);
    if (r != EO_SUCCESS)
        return luaL_error(L, "add_fixed_string failed: %s", eo_result_string(r));
    return 0;
}

static int writer_add_fixed_encoded_string(lua_State *L)
{
    LuaEoWriter *ud = check_writer(L, 1);
    const char *s = luaL_checkstring(L, 2);
    size_t length = (size_t)luaL_checkinteger(L, 3);
    int padded = lua_toboolean(L, 4);
    EoResult r = eo_writer_add_fixed_encoded_string(&ud->writer, s, length, padded ? true : false);
    if (r != EO_SUCCESS)
        return luaL_error(L, "add_fixed_encoded_string failed: %s", eo_result_string(r));
    return 0;
}

static int writer_add_bytes(lua_State *L)
{
    LuaEoWriter *ud = check_writer(L, 1);
    size_t len;
    const char *data = luaL_checklstring(L, 2, &len);
    EoResult r = eo_writer_add_bytes(&ud->writer, (const uint8_t *)data, len);
    if (r != EO_SUCCESS)
        return luaL_error(L, "add_bytes failed: %s", eo_result_string(r));
    return 0;
}

static int writer_to_bytes(lua_State *L)
{
    LuaEoWriter *ud = check_writer(L, 1);
    lua_pushlstring(L, (const char *)ud->writer.data, ud->writer.length);
    return 1;
}

static int writer_get_length(lua_State *L)
{
    LuaEoWriter *ud = check_writer(L, 1);
    lua_pushinteger(L, (lua_Integer)ud->writer.length);
    return 1;
}

static int writer_get_string_sanitization_mode(lua_State *L)
{
    LuaEoWriter *ud = check_writer(L, 1);
    lua_pushboolean(L, eo_writer_get_string_sanitization_mode(&ud->writer) ? 1 : 0);
    return 1;
}

static int writer_set_string_sanitization_mode(lua_State *L)
{
    LuaEoWriter *ud = check_writer(L, 1);
    int enabled = lua_toboolean(L, 2);
    eo_writer_set_string_sanitization_mode(&ud->writer, enabled ? true : false);
    return 0;
}

static const luaL_Reg writer_methods[] = {
    {"add_byte",                     writer_add_byte},
    {"add_char",                     writer_add_char},
    {"add_short",                    writer_add_short},
    {"add_three",                    writer_add_three},
    {"add_int",                      writer_add_int},
    {"add_string",                   writer_add_string},
    {"add_encoded_string",           writer_add_encoded_string},
    {"add_fixed_string",             writer_add_fixed_string},
    {"add_fixed_encoded_string",     writer_add_fixed_encoded_string},
    {"add_bytes",                    writer_add_bytes},
    {"to_bytes",                     writer_to_bytes},
    {"get_length",                   writer_get_length},
    {"get_string_sanitization_mode", writer_get_string_sanitization_mode},
    {"set_string_sanitization_mode", writer_set_string_sanitization_mode},
    {NULL, NULL}
};

/* -------------------------------------------------------------------------
 * EoReader userdata
 * ---------------------------------------------------------------------- */

typedef struct
{
    EoReader reader;
    int string_ref; /* Lua registry reference keeping the source string alive */
} LuaEoReader;

static LuaEoReader *check_reader(lua_State *L, int idx)
{
    return (LuaEoReader *)luaL_checkudata(L, idx, EOLIB_READER_MT);
}

static int reader_new(lua_State *L)
{
    size_t len;
    const char *data = luaL_checklstring(L, 1, &len);

    lua_pushvalue(L, 1);
    int ref = luaL_ref(L, LUA_REGISTRYINDEX);

    LuaEoReader *ud = (LuaEoReader *)lua_newuserdata(L, sizeof(LuaEoReader));
    ud->reader = eo_reader_init((const uint8_t *)data, len);
    ud->string_ref = ref;
    eolib_setmetatable(L, EOLIB_READER_MT);
    return 1;
}

static int reader_gc(lua_State *L)
{
    LuaEoReader *ud = (LuaEoReader *)luaL_checkudata(L, 1, EOLIB_READER_MT);
    luaL_unref(L, LUA_REGISTRYINDEX, ud->string_ref);
    eo_reader_free(&ud->reader);
    return 0;
}

static int reader_get_byte(lua_State *L)
{
    LuaEoReader *ud = check_reader(L, 1);
    uint8_t val = 0;
    EoResult r = eo_reader_get_byte(&ud->reader, &val);
    if (r != EO_SUCCESS)
        return luaL_error(L, "get_byte failed: %s", eo_result_string(r));
    lua_pushinteger(L, (lua_Integer)val);
    return 1;
}

static int reader_get_char(lua_State *L)
{
    LuaEoReader *ud = check_reader(L, 1);
    int32_t val = 0;
    EoResult r = eo_reader_get_char(&ud->reader, &val);
    if (r != EO_SUCCESS)
        return luaL_error(L, "get_char failed: %s", eo_result_string(r));
    lua_pushinteger(L, (lua_Integer)val);
    return 1;
}

static int reader_get_short(lua_State *L)
{
    LuaEoReader *ud = check_reader(L, 1);
    int32_t val = 0;
    EoResult r = eo_reader_get_short(&ud->reader, &val);
    if (r != EO_SUCCESS)
        return luaL_error(L, "get_short failed: %s", eo_result_string(r));
    lua_pushinteger(L, (lua_Integer)val);
    return 1;
}

static int reader_get_three(lua_State *L)
{
    LuaEoReader *ud = check_reader(L, 1);
    int32_t val = 0;
    EoResult r = eo_reader_get_three(&ud->reader, &val);
    if (r != EO_SUCCESS)
        return luaL_error(L, "get_three failed: %s", eo_result_string(r));
    lua_pushinteger(L, (lua_Integer)val);
    return 1;
}

static int reader_get_int(lua_State *L)
{
    LuaEoReader *ud = check_reader(L, 1);
    int32_t val = 0;
    EoResult r = eo_reader_get_int(&ud->reader, &val);
    if (r != EO_SUCCESS)
        return luaL_error(L, "get_int failed: %s", eo_result_string(r));
    lua_pushinteger(L, (lua_Integer)val);
    return 1;
}

static int reader_get_string(lua_State *L)
{
    LuaEoReader *ud = check_reader(L, 1);
    char *val = NULL;
    EoResult r = eo_reader_get_string(&ud->reader, &val);
    if (r != EO_SUCCESS)
        return luaL_error(L, "get_string failed: %s", eo_result_string(r));
    lua_pushstring(L, val ? val : "");
    free(val);
    return 1;
}

static int reader_get_encoded_string(lua_State *L)
{
    LuaEoReader *ud = check_reader(L, 1);
    char *val = NULL;
    EoResult r = eo_reader_get_encoded_string(&ud->reader, &val);
    if (r != EO_SUCCESS)
        return luaL_error(L, "get_encoded_string failed: %s", eo_result_string(r));
    lua_pushstring(L, val ? val : "");
    free(val);
    return 1;
}

static int reader_get_fixed_string(lua_State *L)
{
    LuaEoReader *ud = check_reader(L, 1);
    size_t length = (size_t)luaL_checkinteger(L, 2);
    char *val = NULL;
    EoResult r = eo_reader_get_fixed_string(&ud->reader, length, &val);
    if (r != EO_SUCCESS)
        return luaL_error(L, "get_fixed_string failed: %s", eo_result_string(r));
    lua_pushstring(L, val ? val : "");
    free(val);
    return 1;
}

static int reader_get_fixed_encoded_string(lua_State *L)
{
    LuaEoReader *ud = check_reader(L, 1);
    size_t length = (size_t)luaL_checkinteger(L, 2);
    char *val = NULL;
    EoResult r = eo_reader_get_fixed_encoded_string(&ud->reader, length, &val);
    if (r != EO_SUCCESS)
        return luaL_error(L, "get_fixed_encoded_string failed: %s", eo_result_string(r));
    lua_pushstring(L, val ? val : "");
    free(val);
    return 1;
}

static int reader_get_bytes(lua_State *L)
{
    LuaEoReader *ud = check_reader(L, 1);
    size_t length = (size_t)luaL_checkinteger(L, 2);
    uint8_t *val = NULL;
    EoResult r = eo_reader_get_bytes(&ud->reader, length, &val);
    if (r != EO_SUCCESS)
        return luaL_error(L, "get_bytes failed: %s", eo_result_string(r));
    lua_pushlstring(L, (const char *)val, length);
    free(val);
    return 1;
}

static int reader_remaining(lua_State *L)
{
    LuaEoReader *ud = check_reader(L, 1);
    lua_pushinteger(L, (lua_Integer)eo_reader_remaining(&ud->reader));
    return 1;
}

static int reader_next_chunk(lua_State *L)
{
    LuaEoReader *ud = check_reader(L, 1);
    EoResult r = eo_reader_next_chunk(&ud->reader);
    if (r != EO_SUCCESS)
        return luaL_error(L, "next_chunk failed: %s", eo_result_string(r));
    return 0;
}

static int reader_get_chunked_reading_mode(lua_State *L)
{
    LuaEoReader *ud = check_reader(L, 1);
    lua_pushboolean(L, eo_reader_get_chunked_reading_mode(&ud->reader) ? 1 : 0);
    return 1;
}

static int reader_set_chunked_reading_mode(lua_State *L)
{
    LuaEoReader *ud = check_reader(L, 1);
    int enabled = lua_toboolean(L, 2);
    eo_reader_set_chunked_reading_mode(&ud->reader, enabled ? true : false);
    return 0;
}

static const luaL_Reg reader_methods[] = {
    {"get_byte",                  reader_get_byte},
    {"get_char",                  reader_get_char},
    {"get_short",                 reader_get_short},
    {"get_three",                 reader_get_three},
    {"get_int",                   reader_get_int},
    {"get_string",                reader_get_string},
    {"get_encoded_string",        reader_get_encoded_string},
    {"get_fixed_string",          reader_get_fixed_string},
    {"get_fixed_encoded_string",  reader_get_fixed_encoded_string},
    {"get_bytes",                 reader_get_bytes},
    {"remaining",                 reader_remaining},
    {"next_chunk",                reader_next_chunk},
    {"get_chunked_reading_mode",  reader_get_chunked_reading_mode},
    {"set_chunked_reading_mode",  reader_set_chunked_reading_mode},
    {NULL, NULL}
};

/* -------------------------------------------------------------------------
 * Standalone encoding utilities
 * ---------------------------------------------------------------------- */

static int lua_encode_number(lua_State *L)
{
    int32_t number = (int32_t)luaL_checkinteger(L, 1);
    uint8_t out[4] = {0};
    EoResult r = eo_encode_number(number, out);
    if (r != EO_SUCCESS)
        return luaL_error(L, "encode_number failed: %s", eo_result_string(r));
    lua_pushlstring(L, (const char *)out, 4);
    return 1;
}

static int lua_decode_number(lua_State *L)
{
    size_t len;
    const char *data = luaL_checklstring(L, 1, &len);
    if (len > 4) len = 4;
    int32_t val = eo_decode_number((const uint8_t *)data, len);
    lua_pushinteger(L, (lua_Integer)val);
    return 1;
}

static int lua_encode_string_fn(lua_State *L)
{
    size_t len;
    const char *s = luaL_checklstring(L, 1, &len);
    uint8_t *buf = (uint8_t *)malloc(len);
    if (!buf)
        return luaL_error(L, "allocation failed");
    memcpy(buf, s, len);
    eo_encode_string(buf, len);
    lua_pushlstring(L, (const char *)buf, len);
    free(buf);
    return 1;
}

static int lua_decode_string_fn(lua_State *L)
{
    size_t len;
    const char *s = luaL_checklstring(L, 1, &len);
    uint8_t *buf = (uint8_t *)malloc(len);
    if (!buf)
        return luaL_error(L, "allocation failed");
    memcpy(buf, s, len);
    eo_decode_string(buf, len);
    lua_pushlstring(L, (const char *)buf, len);
    free(buf);
    return 1;
}

static int lua_string_to_windows_1252_fn(lua_State *L)
{
    const char *s = luaL_checkstring(L, 1);
    char *out = NULL;
    EoResult r = eo_string_to_windows_1252(s, &out);
    if (r != EO_SUCCESS)
        return luaL_error(L, "string_to_windows_1252 failed: %s", eo_result_string(r));
    lua_pushstring(L, out ? out : "");
    free(out);
    return 1;
}

/* -------------------------------------------------------------------------
 * Registration
 * ---------------------------------------------------------------------- */

static void register_metatable(lua_State *L, const char *mt_name,
                                const luaL_Reg *methods, lua_CFunction gc_fn)
{
    luaL_newmetatable(L, mt_name);

    /* __index = method table */
    lua_newtable(L);
#if LUA_VERSION_NUM < 502
    luaL_register(L, NULL, methods);
#else
    luaL_setfuncs(L, methods, 0);
#endif
    lua_setfield(L, -2, "__index");

    /* __gc */
    lua_pushcfunction(L, gc_fn);
    lua_setfield(L, -2, "__gc");

    lua_pop(L, 1); /* pop metatable */
}

void lua_data_register(lua_State *L, int module_idx)
{
    register_metatable(L, EOLIB_WRITER_MT, writer_methods, writer_gc);
    register_metatable(L, EOLIB_READER_MT, reader_methods, reader_gc);

    /* EoWriter table */
    lua_newtable(L);
    lua_pushcfunction(L, writer_new);
    lua_setfield(L, -2, "new");
    lua_setfield(L, module_idx, "EoWriter");

    /* EoReader table */
    lua_newtable(L);
    lua_pushcfunction(L, reader_new);
    lua_setfield(L, -2, "new");
    lua_setfield(L, module_idx, "EoReader");

    /* Standalone utilities */
    lua_pushcfunction(L, lua_encode_number);
    lua_setfield(L, module_idx, "encode_number");

    lua_pushcfunction(L, lua_decode_number);
    lua_setfield(L, module_idx, "decode_number");

    lua_pushcfunction(L, lua_encode_string_fn);
    lua_setfield(L, module_idx, "encode_string");

    lua_pushcfunction(L, lua_decode_string_fn);
    lua_setfield(L, module_idx, "decode_string");

    lua_pushcfunction(L, lua_string_to_windows_1252_fn);
    lua_setfield(L, module_idx, "string_to_windows_1252");

    /* Constants */
    lua_pushinteger(L, EO_CHAR_MAX);
    lua_setfield(L, module_idx, "CHAR_MAX");

    lua_pushinteger(L, EO_SHORT_MAX);
    lua_setfield(L, module_idx, "SHORT_MAX");

    lua_pushinteger(L, EO_THREE_MAX);
    lua_setfield(L, module_idx, "THREE_MAX");

    lua_pushinteger(L, (lua_Integer)EO_INT_MAX);
    lua_setfield(L, module_idx, "INT_MAX");

    lua_pushinteger(L, EO_NUMBER_PADDING);
    lua_setfield(L, module_idx, "NUMBER_PADDING");

    lua_pushinteger(L, EO_BREAK_BYTE);
    lua_setfield(L, module_idx, "BREAK_BYTE");
}
