#include "code_gen_lua.h"
#include "code_gen_utils.h"
#include "code_gen_analysis.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static void write_lua_to_c_elements(FILE *f, ElementList *elements, EnumDef *enums,
    size_t ec, StructDef *structs, size_t sc,
    const char *idx_var, const char *out_expr, const char *out_access, int *depth);

static void write_c_to_lua_elements(FILE *f, ElementList *elements, EnumDef *enums,
    size_t ec, StructDef *structs, size_t sc,
    const char *t_var, const char *val_expr, const char *val_access, int *depth);

static void write_lua_to_c_elements(FILE *f, ElementList *elements, EnumDef *enums,
    size_t ec, StructDef *structs, size_t sc,
    const char *idx_var, const char *out_expr, const char *out_access, int *depth)
{
    for (size_t i = 0; i < elements->elements_count; ++i)
    {
        StructElement *element = &elements->elements[i];

        if (element->kind == ELEMENT_FIELD)
        {
            FieldDef *field = &element->as.field;
            if (!field->name || field->value != NULL)
                continue;

            char *snake_name = to_snake_case(field->name);
            char *type_name = normalize_type_name(field->data_type);

            fprintf(f, "    lua_getfield(L, %s, \"%s\");\n", idx_var, snake_name);

            if (strcmp(type_name, "bool") == 0)
            {
                if (field->optional)
                {
                    fprintf(f,
                        "    if (!lua_isnil(L, -1)) { %s%s%s = (bool*)malloc(sizeof(bool));"
                        " if (%s%s%s) *%s%s%s = lua_toboolean(L, -1) ? true : false; }\n",
                        out_expr, out_access, snake_name,
                        out_expr, out_access, snake_name,
                        out_expr, out_access, snake_name);
                }
                else
                {
                    fprintf(f, "    %s%s%s = lua_toboolean(L, -1) ? true : false;\n",
                        out_expr, out_access, snake_name);
                }
            }
            else if (strcmp(type_name, "string") == 0 || strcmp(type_name, "encoded_string") == 0)
            {
                fprintf(f,
                    "    { const char *__s = lua_tostring(L, -1); if (__s) { size_t __n = strlen(__s);"
                    " %s%s%s = (char*)malloc(__n+1); if (%s%s%s) memcpy(%s%s%s, __s, __n+1); } }\n",
                    out_expr, out_access, snake_name,
                    out_expr, out_access, snake_name,
                    out_expr, out_access, snake_name);
            }
            else if (strcmp(type_name, "blob") == 0)
            {
                fprintf(f,
                    "    { size_t __blen; const char *__bd = lua_tolstring(L, -1, &__blen);"
                    " if (__bd) { %s%s%s = (uint8_t*)malloc(__blen); if (%s%s%s)"
                    " { memcpy(%s%s%s, __bd, __blen); %s%s%s_length = __blen; } } }\n",
                    out_expr, out_access, snake_name,
                    out_expr, out_access, snake_name,
                    out_expr, out_access, snake_name,
                    out_expr, out_access, snake_name);
            }
            else if (enum_exists(enums, ec, type_name))
            {
                if (field->optional)
                {
                    fprintf(f,
                        "    if (!lua_isnil(L, -1)) { %s%s%s = (%s*)malloc(sizeof(%s));"
                        " if (%s%s%s) *%s%s%s = (%s)lua_tointeger(L, -1); }\n",
                        out_expr, out_access, snake_name, type_name, type_name,
                        out_expr, out_access, snake_name,
                        out_expr, out_access, snake_name, type_name);
                }
                else
                {
                    fprintf(f, "    %s%s%s = (%s)lua_tointeger(L, -1);\n",
                        out_expr, out_access, snake_name, type_name);
                }
            }
            else if (struct_exists(structs, sc, type_name))
            {
                if (struct_has_storage(structs, sc, type_name))
                {
                    if (field->optional)
                    {
                        fprintf(f,
                            "    if (!lua_isnil(L, -1) && lua_istable(L, -1)) {"
                            " %s%s%s = (%s*)calloc(1, sizeof(%s));"
                            " if (%s%s%s) lua_table_to_%s(L, lua_gettop(L), %s%s%s); }\n",
                            out_expr, out_access, snake_name, type_name, type_name,
                            out_expr, out_access, snake_name, type_name,
                            out_expr, out_access, snake_name);
                    }
                    else
                    {
                        fprintf(f,
                            "    if (lua_istable(L, -1)) lua_table_to_%s(L, lua_gettop(L), &%s%s%s);\n",
                            type_name, out_expr, out_access, snake_name);
                    }
                }
            }
            else
            {
                const char *c_type = map_primitive_type(type_name);
                if (field->optional)
                {
                    fprintf(f,
                        "    if (!lua_isnil(L, -1)) { %s%s%s = (%s*)malloc(sizeof(%s));"
                        " if (%s%s%s) *%s%s%s = (%s)lua_tointeger(L, -1); }\n",
                        out_expr, out_access, snake_name, c_type, c_type,
                        out_expr, out_access, snake_name,
                        out_expr, out_access, snake_name, c_type);
                }
                else
                {
                    fprintf(f, "    %s%s%s = (%s)lua_tointeger(L, -1);\n",
                        out_expr, out_access, snake_name, c_type);
                }
            }

            fprintf(f, "    lua_pop(L, 1);\n");
            free(snake_name);
            free(type_name);
        }
        else if (element->kind == ELEMENT_ARRAY)
        {
            ArrayDef *array = &element->as.array;
            if (!array->name)
                continue;

            int d = *depth;
            (*depth)++;

            char *arr_name = to_snake_case(array->name);
            char *type_name = normalize_type_name(array->data_type);
            int is_static = is_static_length(array->length);

            int is_string_t = (strcmp(type_name, "string") == 0 ||
                               strcmp(type_name, "encoded_string") == 0);
            int is_enum_t = enum_exists(enums, ec, type_name);
            int is_struct_t = (!is_primitive_type(type_name) && !is_enum_t &&
                               struct_exists(structs, sc, type_name));

            const char *c_type;
            if (is_string_t)
                c_type = "char*";
            else if (strcmp(type_name, "blob") == 0)
                c_type = "uint8_t";
            else if (is_struct_t || is_enum_t)
                c_type = type_name;
            else
                c_type = map_primitive_type(type_name);

            fprintf(f, "    lua_getfield(L, %s, \"%s\");\n", idx_var, arr_name);
            fprintf(f,
                "    if (lua_istable(L, -1)) { int __eolib_arr_%d = lua_gettop(L);"
                " int __eolib_n_%d = (int)lua_rawlen(L, __eolib_arr_%d);\n",
                d, d, d);

            if (is_static)
            {
                fprintf(f,
                    "    int __eolib_lim_%d = %s; if (__eolib_n_%d > __eolib_lim_%d)"
                    " __eolib_n_%d = __eolib_lim_%d;\n",
                    d, array->length, d, d, d, d);
            }
            else
            {
                fprintf(f,
                    "    %s%s%s_length = (size_t)__eolib_n_%d;"
                    " %s%s%s = __eolib_n_%d > 0 ? (%s*)calloc((size_t)__eolib_n_%d, sizeof(%s)) : NULL;\n",
                    out_expr, out_access, arr_name, d,
                    out_expr, out_access, arr_name, d,
                    c_type, d, c_type);
            }

            fprintf(f,
                "    for (__eolib_i = 1; __eolib_i <= __eolib_n_%d; __eolib_i++) {"
                " lua_rawgeti(L, __eolib_arr_%d, __eolib_i);\n",
                d, d);

            if (is_string_t)
            {
                if (is_static)
                {
                    fprintf(f,
                        "    { const char *__s = lua_tostring(L, -1); if (__s) { size_t __n = strlen(__s);"
                        " %s%s%s[__eolib_i - 1] = (char*)malloc(__n+1);"
                        " if (%s%s%s[__eolib_i - 1]) memcpy(%s%s%s[__eolib_i - 1], __s, __n+1); } }\n",
                        out_expr, out_access, arr_name,
                        out_expr, out_access, arr_name,
                        out_expr, out_access, arr_name);
                }
                else
                {
                    fprintf(f,
                        "    if (%s%s%s) { const char *__s = lua_tostring(L, -1); if (__s) {"
                        " size_t __n = strlen(__s); %s%s%s[__eolib_i - 1] = (char*)malloc(__n+1);"
                        " if (%s%s%s[__eolib_i - 1]) memcpy(%s%s%s[__eolib_i - 1], __s, __n+1); } }\n",
                        out_expr, out_access, arr_name,
                        out_expr, out_access, arr_name,
                        out_expr, out_access, arr_name,
                        out_expr, out_access, arr_name);
                }
            }
            else if (strcmp(type_name, "bool") == 0)
            {
                if (is_static)
                {
                    fprintf(f,
                        "    %s%s%s[__eolib_i - 1] = lua_toboolean(L, -1) ? true : false;\n",
                        out_expr, out_access, arr_name);
                }
                else
                {
                    fprintf(f,
                        "    if (%s%s%s) %s%s%s[__eolib_i - 1] = lua_toboolean(L, -1) ? true : false;\n",
                        out_expr, out_access, arr_name,
                        out_expr, out_access, arr_name);
                }
            }
            else if (is_struct_t)
            {
                if (struct_has_storage(structs, sc, type_name))
                {
                    if (is_static)
                    {
                        fprintf(f,
                            "    if (lua_istable(L, -1)) lua_table_to_%s(L, lua_gettop(L), &%s%s%s[__eolib_i - 1]);\n",
                            type_name, out_expr, out_access, arr_name);
                    }
                    else
                    {
                        fprintf(f,
                            "    if (%s%s%s && lua_istable(L, -1)) lua_table_to_%s(L, lua_gettop(L), &%s%s%s[__eolib_i - 1]);\n",
                            out_expr, out_access, arr_name, type_name,
                            out_expr, out_access, arr_name);
                    }
                }
            }
            else if (is_enum_t)
            {
                if (is_static)
                {
                    fprintf(f,
                        "    %s%s%s[__eolib_i - 1] = (%s)lua_tointeger(L, -1);\n",
                        out_expr, out_access, arr_name, type_name);
                }
                else
                {
                    fprintf(f,
                        "    if (%s%s%s) %s%s%s[__eolib_i - 1] = (%s)lua_tointeger(L, -1);\n",
                        out_expr, out_access, arr_name,
                        out_expr, out_access, arr_name, type_name);
                }
            }
            else
            {
                if (is_static)
                {
                    fprintf(f,
                        "    %s%s%s[__eolib_i - 1] = (%s)lua_tointeger(L, -1);\n",
                        out_expr, out_access, arr_name, c_type);
                }
                else
                {
                    fprintf(f,
                        "    if (%s%s%s) %s%s%s[__eolib_i - 1] = (%s)lua_tointeger(L, -1);\n",
                        out_expr, out_access, arr_name,
                        out_expr, out_access, arr_name, c_type);
                }
            }

            fprintf(f, "    lua_pop(L, 1); }\n");
            fprintf(f, "    }\n");
            fprintf(f, "    lua_pop(L, 1);\n");

            free(arr_name);
            free(type_name);
        }
        else if (element->kind == ELEMENT_SWITCH)
        {
            SwitchDef *sw = &element->as.sw;
            if (!switch_has_storage(sw))
                continue;

            int d = *depth;
            (*depth)++;

            char *field_name = to_snake_case(sw->field);
            char *field_type = find_field_data_type(elements, sw->field);

            char di_var[64];
            snprintf(di_var, sizeof(di_var), "__eolib_di_%d", d);

            fprintf(f, "    lua_getfield(L, %s, \"%s_data\");\n", idx_var, field_name);
            fprintf(f,
                "    if (lua_istable(L, -1)) { int %s = lua_gettop(L);"
                " %s%s%s_data = calloc(1, sizeof(*%s%s%s_data)); if (%s%s%s_data) {\n",
                di_var,
                out_expr, out_access, field_name,
                out_expr, out_access, field_name,
                out_expr, out_access, field_name);
            write_switch_pragma_push(f);
            fprintf(f, "    switch (%s%s%s) {\n", out_expr, out_access, field_name);

            int has_default = 0;
            for (size_t c = 0; c < sw->cases_count; ++c)
            {
                CaseDef *case_def = &sw->cases[c];
                if (!case_def->has_elements || !element_list_has_storage(&case_def->elements))
                    continue;

                if (case_def->is_default)
                    has_default = 1;

                const char *case_name = case_def->is_default ? "default" : case_def->value;
                char *case_const = to_upper_snake(case_name);
                char *case_field = to_snake_case(case_name);

                if (case_def->is_default)
                {
                    fprintf(f, "    default: {\n");
                }
                else if (is_numeric_string(case_def->value))
                {
                    fprintf(f, "    case %s: {\n", case_def->value);
                }
                else if (field_type && enum_exists(enums, ec, field_type))
                {
                    char *enum_prefix = to_upper_snake(field_type);
                    fprintf(f, "    case %s_%s: {\n", enum_prefix, case_const);
                    free(enum_prefix);
                }
                else
                {
                    fprintf(f, "    case %s: {\n", case_def->value ? case_def->value : "0");
                }

                char new_out_expr[1024];
                snprintf(new_out_expr, sizeof(new_out_expr), "%s%s%s_data->%s",
                    out_expr, out_access, field_name, case_field);
                write_lua_to_c_elements(f, &case_def->elements, enums, ec, structs, sc,
                    di_var, new_out_expr, ".", depth);

                fprintf(f, "    break; }\n");
                free(case_const);
                free(case_field);
            }

            if (!has_default)
                fprintf(f, "    default: break;\n");
            fprintf(f, "    }\n");
            write_switch_pragma_pop(f);
            fprintf(f, "    } }\n");
            fprintf(f, "    lua_pop(L, 1);\n");

            free(field_name);
            free(field_type);
        }
        else if (element->kind == ELEMENT_CHUNKED)
        {
            write_lua_to_c_elements(f, &element->as.chunked, enums, ec, structs, sc,
                idx_var, out_expr, out_access, depth);
        }
    }
}

static void write_c_to_lua_elements(FILE *f, ElementList *elements, EnumDef *enums,
    size_t ec, StructDef *structs, size_t sc,
    const char *t_var, const char *val_expr, const char *val_access, int *depth)
{
    for (size_t i = 0; i < elements->elements_count; ++i)
    {
        StructElement *element = &elements->elements[i];

        if (element->kind == ELEMENT_FIELD)
        {
            FieldDef *field = &element->as.field;
            if (!field->name || field->value != NULL)
                continue;

            char *snake_name = to_snake_case(field->name);
            char *type_name = normalize_type_name(field->data_type);

            if (strcmp(type_name, "bool") == 0)
            {
                if (field->optional)
                {
                    fprintf(f,
                        "    if (%s%s%s) lua_pushboolean(L, *%s%s%s ? 1 : 0);"
                        " else lua_pushnil(L); lua_setfield(L, %s, \"%s\");\n",
                        val_expr, val_access, snake_name,
                        val_expr, val_access, snake_name,
                        t_var, snake_name);
                }
                else
                {
                    fprintf(f,
                        "    lua_pushboolean(L, %s%s%s ? 1 : 0); lua_setfield(L, %s, \"%s\");\n",
                        val_expr, val_access, snake_name, t_var, snake_name);
                }
            }
            else if (strcmp(type_name, "string") == 0 || strcmp(type_name, "encoded_string") == 0)
            {
                fprintf(f,
                    "    lua_pushstring(L, %s%s%s ? %s%s%s : \"\"); lua_setfield(L, %s, \"%s\");\n",
                    val_expr, val_access, snake_name,
                    val_expr, val_access, snake_name,
                    t_var, snake_name);
            }
            else if (strcmp(type_name, "blob") == 0)
            {
                fprintf(f,
                    "    lua_pushlstring(L, (const char*)%s%s%s, %s%s%s_length);"
                    " lua_setfield(L, %s, \"%s\");\n",
                    val_expr, val_access, snake_name,
                    val_expr, val_access, snake_name,
                    t_var, snake_name);
            }
            else if (enum_exists(enums, ec, type_name))
            {
                if (field->optional)
                {
                    fprintf(f,
                        "    if (%s%s%s) lua_pushinteger(L, (lua_Integer)*%s%s%s);"
                        " else lua_pushnil(L); lua_setfield(L, %s, \"%s\");\n",
                        val_expr, val_access, snake_name,
                        val_expr, val_access, snake_name,
                        t_var, snake_name);
                }
                else
                {
                    fprintf(f,
                        "    lua_pushinteger(L, (lua_Integer)%s%s%s); lua_setfield(L, %s, \"%s\");\n",
                        val_expr, val_access, snake_name, t_var, snake_name);
                }
            }
            else if (struct_exists(structs, sc, type_name))
            {
                if (struct_has_storage(structs, sc, type_name))
                {
                    if (field->optional)
                    {
                        fprintf(f,
                            "    if (%s%s%s) { %s_to_lua_table(L, %s%s%s); }"
                            " else { lua_pushnil(L); } lua_setfield(L, %s, \"%s\");\n",
                            val_expr, val_access, snake_name,
                            type_name, val_expr, val_access, snake_name,
                            t_var, snake_name);
                    }
                    else
                    {
                        fprintf(f,
                            "    %s_to_lua_table(L, &%s%s%s); lua_setfield(L, %s, \"%s\");\n",
                            type_name, val_expr, val_access, snake_name, t_var, snake_name);
                    }
                }
            }
            else
            {
                if (field->optional)
                {
                    fprintf(f,
                        "    if (%s%s%s) lua_pushinteger(L, (lua_Integer)*%s%s%s);"
                        " else lua_pushnil(L); lua_setfield(L, %s, \"%s\");\n",
                        val_expr, val_access, snake_name,
                        val_expr, val_access, snake_name,
                        t_var, snake_name);
                }
                else
                {
                    fprintf(f,
                        "    lua_pushinteger(L, (lua_Integer)%s%s%s); lua_setfield(L, %s, \"%s\");\n",
                        val_expr, val_access, snake_name, t_var, snake_name);
                }
            }

            free(snake_name);
            free(type_name);
        }
        else if (element->kind == ELEMENT_ARRAY)
        {
            ArrayDef *array = &element->as.array;
            if (!array->name)
                continue;

            int d = *depth;
            (*depth)++;

            char *arr_name = to_snake_case(array->name);
            char *type_name = normalize_type_name(array->data_type);
            int is_static = is_static_length(array->length);

            int is_string_t = (strcmp(type_name, "string") == 0 ||
                               strcmp(type_name, "encoded_string") == 0);
            int is_enum_t = enum_exists(enums, ec, type_name);
            int is_struct_t = (!is_primitive_type(type_name) && !is_enum_t &&
                               struct_exists(structs, sc, type_name));

            fprintf(f, "    { lua_newtable(L); int __eolib_arrt_%d = lua_gettop(L);\n", d);

            if (is_static)
            {
                fprintf(f,
                    "    for (int __eolib_ai_%d = 0; __eolib_ai_%d < %s; __eolib_ai_%d++) {\n",
                    d, d, array->length, d);
            }
            else
            {
                fprintf(f,
                    "    for (size_t __eolib_ai_%d = 0; __eolib_ai_%d < %s%s%s_length; __eolib_ai_%d++) {\n",
                    d, d, val_expr, val_access, arr_name, d);
            }

            if (is_string_t)
            {
                fprintf(f,
                    "    lua_pushstring(L, %s%s%s[__eolib_ai_%d] ? %s%s%s[__eolib_ai_%d] : \"\");\n",
                    val_expr, val_access, arr_name, d,
                    val_expr, val_access, arr_name, d);
            }
            else if (strcmp(type_name, "bool") == 0)
            {
                fprintf(f,
                    "    lua_pushboolean(L, %s%s%s[__eolib_ai_%d] ? 1 : 0);\n",
                    val_expr, val_access, arr_name, d);
            }
            else if (is_struct_t)
            {
                if (struct_has_storage(structs, sc, type_name))
                {
                    fprintf(f,
                        "    %s_to_lua_table(L, &%s%s%s[__eolib_ai_%d]);\n",
                        type_name, val_expr, val_access, arr_name, d);
                }
                else
                {
                    fprintf(f, "    lua_pushnil(L);\n");
                }
            }
            else
            {
                fprintf(f,
                    "    lua_pushinteger(L, (lua_Integer)%s%s%s[__eolib_ai_%d]);\n",
                    val_expr, val_access, arr_name, d);
            }

            fprintf(f, "    lua_rawseti(L, __eolib_arrt_%d, (int)__eolib_ai_%d + 1); }\n", d, d);
            fprintf(f, "    lua_setfield(L, %s, \"%s\"); }\n", t_var, arr_name);

            free(arr_name);
            free(type_name);
        }
        else if (element->kind == ELEMENT_SWITCH)
        {
            SwitchDef *sw = &element->as.sw;
            if (!switch_has_storage(sw))
                continue;

            int d = *depth;
            (*depth)++;

            char *field_name = to_snake_case(sw->field);
            char *field_type = find_field_data_type(elements, sw->field);

            char dt_var[64];
            snprintf(dt_var, sizeof(dt_var), "__eolib_dt_%d", d);

            fprintf(f,
                "    if (%s%s%s_data) { lua_newtable(L); int %s = lua_gettop(L);\n",
                val_expr, val_access, field_name, dt_var);
            write_switch_pragma_push(f);
            fprintf(f, "    switch (%s%s%s) {\n", val_expr, val_access, field_name);

            int has_default = 0;
            for (size_t c = 0; c < sw->cases_count; ++c)
            {
                CaseDef *case_def = &sw->cases[c];
                if (!case_def->has_elements || !element_list_has_storage(&case_def->elements))
                    continue;

                if (case_def->is_default)
                    has_default = 1;

                const char *case_name = case_def->is_default ? "default" : case_def->value;
                char *case_const = to_upper_snake(case_name);
                char *case_field = to_snake_case(case_name);

                if (case_def->is_default)
                {
                    fprintf(f, "    default: {\n");
                }
                else if (is_numeric_string(case_def->value))
                {
                    fprintf(f, "    case %s: {\n", case_def->value);
                }
                else if (field_type && enum_exists(enums, ec, field_type))
                {
                    char *enum_prefix = to_upper_snake(field_type);
                    fprintf(f, "    case %s_%s: {\n", enum_prefix, case_const);
                    free(enum_prefix);
                }
                else
                {
                    fprintf(f, "    case %s: {\n", case_def->value ? case_def->value : "0");
                }

                char new_val_expr[1024];
                snprintf(new_val_expr, sizeof(new_val_expr), "%s%s%s_data->%s",
                    val_expr, val_access, field_name, case_field);
                write_c_to_lua_elements(f, &case_def->elements, enums, ec, structs, sc,
                    dt_var, new_val_expr, ".", depth);

                fprintf(f, "    break; }\n");
                free(case_const);
                free(case_field);
            }

            if (!has_default)
                fprintf(f, "    default: break;\n");
            fprintf(f, "    }\n");
            write_switch_pragma_pop(f);
            fprintf(f,
                "    lua_setfield(L, %s, \"%s_data\"); }"
                " else { lua_pushnil(L); lua_setfield(L, %s, \"%s_data\"); }\n",
                t_var, field_name, t_var, field_name);

            free(field_name);
            free(field_type);
        }
        else if (element->kind == ELEMENT_CHUNKED)
        {
            write_c_to_lua_elements(f, &element->as.chunked, enums, ec, structs, sc,
                t_var, val_expr, val_access, depth);
        }
    }
}

static void write_lua_enum_registration(FILE *f, EnumDef *def)
{
    fprintf(f, "static void lua_register_%s(lua_State *L, int module_idx)\n{\n", def->name);
    fprintf(f, "    lua_newtable(L);\n");
    for (size_t i = 0; i < def->values_count; ++i)
    {
        EnumValue *val = &def->values[i];
        fprintf(f, "    lua_pushinteger(L, %d); lua_setfield(L, -2, \"%s\");\n",
            val->value, val->name);
    }
    fprintf(f, "    lua_setfield(L, module_idx, \"%s\");\n}\n\n", def->name);
}

static void write_lua_type_bindings(FILE *f, const char *name, ElementList *elements,
    EnumDef *enums, size_t ec, StructDef *structs, size_t sc)
{
    int has_storage = element_list_has_storage(elements);
    int has_heap = element_list_has_heap(elements, enums, ec, structs, sc);

    if (has_storage)
    {
        fprintf(f, "static void lua_table_to_%s(lua_State *L, int idx, %s *out)\n{\n",
            name, name);
        fprintf(f, "    (void)L; (void)out;\n");
        fprintf(f, "    int __eolib_i; (void)__eolib_i;\n");
        int depth = 0;
        write_lua_to_c_elements(f, elements, enums, ec, structs, sc, "idx", "out", "->", &depth);
        fprintf(f, "}\n\n");

        fprintf(f, "static void %s_to_lua_table(lua_State *L, const %s *val)\n{\n", name, name);
        fprintf(f, "    lua_newtable(L); int __t = lua_gettop(L); (void)val;\n");
        depth = 0;
        write_c_to_lua_elements(f, elements, enums, ec, structs, sc, "__t", "val", "->", &depth);
        fprintf(f, "}\n\n");

        fprintf(f, "static int lua_%s_new(lua_State *L)\n{\n", name);
        fprintf(f, "    (void)L;\n");
        fprintf(f, "    lua_newtable(L);\n");
        fprintf(f, "    eolib_setmetatable(L, \"eolib.%s\");\n", name);
        fprintf(f, "    return 1;\n}\n\n");

        fprintf(f, "static int lua_%s_serialize(lua_State *L)\n{\n", name);
        fprintf(f, "    luaL_checktype(L, 1, LUA_TTABLE);\n");
        fprintf(f, "    %s __val;\n", name);
        fprintf(f, "    memset(&__val, 0, sizeof(__val));\n");
        fprintf(f, "    lua_table_to_%s(L, 1, &__val);\n", name);
        fprintf(f, "    EoWriter __writer = eo_writer_init_with_capacity(%s_size(&__val));\n", name);
        fprintf(f, "    EoResult __r = %s_serialize(&__val, &__writer);\n", name);
        if (has_heap)
            fprintf(f, "    %s_free(&__val);\n", name);
        fprintf(f,
            "    if (__r != EO_SUCCESS) { eo_writer_free(&__writer);"
            " return luaL_error(L, \"serialize failed: %%s\", eo_result_string(__r)); }\n");
        fprintf(f, "    lua_pushlstring(L, (const char *)__writer.data, __writer.length);\n");
        fprintf(f, "    eo_writer_free(&__writer);\n");
        fprintf(f, "    return 1;\n}\n\n");

        fprintf(f, "static int lua_%s_deserialize(lua_State *L)\n{\n", name);
        fprintf(f, "    size_t __len;\n");
        fprintf(f, "    const char *__data = luaL_checklstring(L, 1, &__len);\n");
        fprintf(f, "    EoReader __reader = eo_reader_init((const uint8_t *)__data, __len);\n");
        fprintf(f, "    %s __val;\n", name);
        fprintf(f, "    memset(&__val, 0, sizeof(__val));\n");
        fprintf(f, "    EoResult __r = %s_deserialize(&__val, &__reader);\n", name);
        fprintf(f,
            "    if (__r != EO_SUCCESS) { return luaL_error(L, \"deserialize failed: %%s\","
            " eo_result_string(__r)); }\n");
        fprintf(f, "    %s_to_lua_table(L, &__val);\n", name);
        fprintf(f, "    eolib_setmetatable(L, \"eolib.%s\");\n", name);
        if (has_heap)
            fprintf(f, "    %s_free(&__val);\n", name);
        fprintf(f, "    return 1;\n}\n\n");

        fprintf(f, "static int lua_%s_write(lua_State *L)\n{\n", name);
        fprintf(f, "    luaL_checktype(L, 1, LUA_TTABLE);\n");
        fprintf(f,
            "    LuaEoWriter *__uw = (LuaEoWriter *)luaL_checkudata(L, 2, EOLIB_WRITER_MT);\n");
        fprintf(f, "    %s __val;\n", name);
        fprintf(f, "    memset(&__val, 0, sizeof(__val));\n");
        fprintf(f, "    lua_table_to_%s(L, 1, &__val);\n", name);
        fprintf(f, "    EoResult __r = %s_serialize(&__val, &__uw->writer);\n", name);
        if (has_heap)
            fprintf(f, "    %s_free(&__val);\n", name);
        fprintf(f,
            "    if (__r != EO_SUCCESS) { return luaL_error(L, \"write failed: %%s\","
            " eo_result_string(__r)); }\n");
        fprintf(f, "    return 0;\n}\n\n");

        fprintf(f, "static void lua_register_%s(lua_State *L, int module_idx)\n{\n", name);
        fprintf(f, "    luaL_newmetatable(L, \"eolib.%s\");\n", name);
        fprintf(f, "    lua_newtable(L);\n");
        fprintf(f,
            "    lua_pushcfunction(L, lua_%s_serialize); lua_setfield(L, -2, \"serialize\");\n",
            name);
        fprintf(f,
            "    lua_pushcfunction(L, lua_%s_write); lua_setfield(L, -2, \"write\");\n",
            name);
        fprintf(f, "    lua_setfield(L, -2, \"__index\");\n");
        fprintf(f, "    lua_pop(L, 1);\n\n");
        fprintf(f, "    lua_newtable(L);\n");
        fprintf(f,
            "    lua_pushcfunction(L, lua_%s_new); lua_setfield(L, -2, \"new\");\n", name);
        fprintf(f,
            "    lua_pushcfunction(L, lua_%s_deserialize); lua_setfield(L, -2, \"deserialize\");\n",
            name);
        fprintf(f, "    lua_setfield(L, module_idx, \"%s\");\n}\n\n", name);
    }
    else
    {
        fprintf(f, "static int lua_%s_serialize(lua_State *L)\n{\n", name);
        fprintf(f, "    (void)L;\n");
        fprintf(f, "    EoWriter __writer = eo_writer_init_with_capacity(%s_size());\n", name);
        fprintf(f, "    EoResult __r = %s_serialize(&__writer);\n", name);
        fprintf(f,
            "    if (__r != EO_SUCCESS) { eo_writer_free(&__writer);"
            " return luaL_error(L, \"serialize failed: %%s\", eo_result_string(__r)); }\n");
        fprintf(f, "    lua_pushlstring(L, (const char *)__writer.data, __writer.length);\n");
        fprintf(f, "    eo_writer_free(&__writer);\n");
        fprintf(f, "    return 1;\n}\n\n");

        fprintf(f, "static int lua_%s_deserialize(lua_State *L)\n{\n", name);
        fprintf(f, "    size_t __len;\n");
        fprintf(f, "    const char *__data = luaL_checklstring(L, 1, &__len);\n");
        fprintf(f, "    EoReader __reader = eo_reader_init((const uint8_t *)__data, __len);\n");
        fprintf(f, "    EoResult __r = %s_deserialize(&__reader);\n", name);
        fprintf(f,
            "    if (__r != EO_SUCCESS) { return luaL_error(L, \"deserialize failed: %%s\","
            " eo_result_string(__r)); }\n");
        fprintf(f, "    lua_pushboolean(L, 1);\n");
        fprintf(f, "    return 1;\n}\n\n");

        fprintf(f, "static void lua_register_%s(lua_State *L, int module_idx)\n{\n", name);
        fprintf(f, "    lua_newtable(L);\n");
        fprintf(f,
            "    lua_pushcfunction(L, lua_%s_serialize); lua_setfield(L, -2, \"serialize\");\n",
            name);
        fprintf(f,
            "    lua_pushcfunction(L, lua_%s_deserialize); lua_setfield(L, -2, \"deserialize\");\n",
            name);
        fprintf(f, "    lua_setfield(L, module_idx, \"%s\");\n}\n\n", name);
    }
}

void write_lua_files(ProtocolDef *protocols, size_t protocol_count)
{
    ensure_dir("lua");
    ensure_dir("lua/src");

    FILE *header = fopen("lua/src/lua_protocol.h", "w");
    FILE *source = fopen("lua/src/lua_protocol.c", "w");

    if (!header || !source)
    {
        fprintf(stderr, "Failed to open lua protocol output files\n");
        if (header)
            fclose(header);
        if (source)
            fclose(source);
        return;
    }

    fprintf(header, "%s", CODEGEN_WARNING);
    fprintf(header, "#ifndef EOLIB_LUA_PROTOCOL_H\n#define EOLIB_LUA_PROTOCOL_H\n\n");
    fprintf(header, "#include <lua.h>\n\n");
    fprintf(header, "void lua_protocol_register(lua_State *L, int module_idx);\n\n");
    fprintf(header, "#endif /* EOLIB_LUA_PROTOCOL_H */\n");
    fclose(header);

    EnumDef *all_enums = NULL;
    size_t all_enums_count = 0;
    StructDef *all_structs = NULL;
    size_t all_structs_count = 0;
    collect_all_enums_structs(protocols, protocol_count, &all_enums, &all_enums_count,
                              &all_structs, &all_structs_count);

    fprintf(source, "%s", CODEGEN_WARNING);
    fprintf(source, "#include \"lua_protocol.h\"\n");
    fprintf(source, "#include \"eolib_lua.h\"\n");
    fprintf(source, "#include \"eolib/protocol.h\"\n");
    fprintf(source, "#include \"eolib/result.h\"\n");
    fprintf(source, "#include \"eolib/data.h\"\n");
    fprintf(source, "#include <stdlib.h>\n#include <string.h>\n\n");

    StringList written_fwd = {0};

    for (size_t p = 0; p < protocol_count; ++p)
    {
        ProtocolDef *protocol = &protocols[p];
        for (size_t i = 0; i < protocol->structs_count; ++i)
        {
            const char *sname = protocol->structs[i].name;
            if (string_list_contains(&written_fwd, sname))
                continue;
            if (element_list_has_storage(&protocol->structs[i].elements))
            {
                fprintf(source,
                    "static void lua_table_to_%s(lua_State *L, int idx, %s *out);\n",
                    sname, sname);
                fprintf(source,
                    "static void %s_to_lua_table(lua_State *L, const %s *val);\n",
                    sname, sname);
            }
            string_list_push(&written_fwd, sname);
        }
    }

    for (size_t p = 0; p < protocol_count; ++p)
    {
        ProtocolDef *protocol = &protocols[p];
        const char *source_suffix = "Client";
        if (strstr(protocol->path, "/server/"))
            source_suffix = "Server";
        else if (strstr(protocol->path, "/client/"))
            source_suffix = "Client";

        for (size_t i = 0; i < protocol->packets_count; ++i)
        {
            char name_buffer[256];
            snprintf(name_buffer, sizeof(name_buffer), "%s%s%sPacket",
                protocol->packets[i].family, protocol->packets[i].action, source_suffix);
            if (string_list_contains(&written_fwd, name_buffer))
                continue;
            if (element_list_has_storage(&protocol->packets[i].elements))
            {
                fprintf(source,
                    "static void lua_table_to_%s(lua_State *L, int idx, %s *out);\n",
                    name_buffer, name_buffer);
                fprintf(source,
                    "static void %s_to_lua_table(lua_State *L, const %s *val);\n",
                    name_buffer, name_buffer);
            }
            string_list_push(&written_fwd, name_buffer);
        }
    }
    fprintf(source, "\n");

    StringList written_enums = {0};
    for (size_t p = 0; p < protocol_count; ++p)
    {
        ProtocolDef *protocol = &protocols[p];
        for (size_t i = 0; i < protocol->enums_count; ++i)
        {
            if (string_list_contains(&written_enums, protocol->enums[i].name))
                continue;
            write_lua_enum_registration(source, &protocol->enums[i]);
            string_list_push(&written_enums, protocol->enums[i].name);
        }
    }

    StringList written_structs = {0};
    for (size_t p = 0; p < protocol_count; ++p)
    {
        ProtocolDef *protocol = &protocols[p];
        for (size_t i = 0; i < protocol->structs_count; ++i)
        {
            if (string_list_contains(&written_structs, protocol->structs[i].name))
                continue;
            write_lua_type_bindings(source, protocol->structs[i].name,
                &protocol->structs[i].elements,
                all_enums, all_enums_count, all_structs, all_structs_count);
            string_list_push(&written_structs, protocol->structs[i].name);
        }
    }

    StringList written_packets = {0};
    for (size_t p = 0; p < protocol_count; ++p)
    {
        ProtocolDef *protocol = &protocols[p];
        const char *source_suffix = "Client";
        if (strstr(protocol->path, "/server/"))
            source_suffix = "Server";
        else if (strstr(protocol->path, "/client/"))
            source_suffix = "Client";

        for (size_t i = 0; i < protocol->packets_count; ++i)
        {
            char name_buffer[256];
            snprintf(name_buffer, sizeof(name_buffer), "%s%s%sPacket",
                protocol->packets[i].family, protocol->packets[i].action, source_suffix);
            if (string_list_contains(&written_packets, name_buffer))
                continue;
            write_lua_type_bindings(source, name_buffer,
                &protocol->packets[i].elements,
                all_enums, all_enums_count, all_structs, all_structs_count);
            string_list_push(&written_packets, name_buffer);
        }
    }

    fprintf(source, "void lua_protocol_register(lua_State *L, int module_idx)\n{\n");
    for (size_t i = 0; i < written_enums.count; ++i)
        fprintf(source, "    lua_register_%s(L, module_idx);\n", written_enums.items[i]);
    for (size_t i = 0; i < written_structs.count; ++i)
        fprintf(source, "    lua_register_%s(L, module_idx);\n", written_structs.items[i]);
    for (size_t i = 0; i < written_packets.count; ++i)
        fprintf(source, "    lua_register_%s(L, module_idx);\n", written_packets.items[i]);
    fprintf(source, "}\n");

    fclose(source);

    string_list_free(&written_fwd);
    string_list_free(&written_enums);
    string_list_free(&written_structs);
    string_list_free(&written_packets);
    free(all_enums);
    free(all_structs);
}

static const char *lua_cats_type(const char *data_type,
    EnumDef *enums, size_t enums_count,
    StructDef *structs, size_t structs_count)
{
    if (!data_type) return "any";
    if (strcmp(data_type, "byte")   == 0 ||
        strcmp(data_type, "char")   == 0 ||
        strcmp(data_type, "short")  == 0 ||
        strcmp(data_type, "three")  == 0 ||
        strcmp(data_type, "int")    == 0) return "integer";
    if (strcmp(data_type, "string")               == 0 ||
        strcmp(data_type, "encoded_string")       == 0 ||
        strcmp(data_type, "fixed_string")         == 0 ||
        strcmp(data_type, "fixed_encoded_string") == 0 ||
        strcmp(data_type, "blob")                 == 0) return "string";
    if (strcmp(data_type, "bool") == 0) return "boolean";
    for (size_t i = 0; i < enums_count; ++i)
        if (strcmp(enums[i].name, data_type) == 0) return data_type;
    for (size_t i = 0; i < structs_count; ++i)
        if (strcmp(structs[i].name, data_type) == 0) return data_type;
    return "any";
}

static void write_cats_elements(FILE *f, ElementList *elements,
    EnumDef *enums, size_t enums_count,
    StructDef *structs, size_t structs_count)
{
    for (size_t i = 0; i < elements->elements_count; ++i)
    {
        StructElement *el = &elements->elements[i];
        if (el->kind == ELEMENT_FIELD)
        {
            FieldDef *fd = &el->as.field;
            if (!fd->name) continue;
            const char *t = lua_cats_type(fd->data_type, enums, enums_count, structs, structs_count);
            if (fd->comment) write_lua_doc_comment(f, fd->comment);
            fprintf(f, "---@field %s %s%s\n", fd->name, t, fd->optional ? "?" : "");
        }
        else if (el->kind == ELEMENT_ARRAY)
        {
            ArrayDef *ad = &el->as.array;
            if (!ad->name) continue;
            const char *t = lua_cats_type(ad->data_type, enums, enums_count, structs, structs_count);
            if (ad->comment) write_lua_doc_comment(f, ad->comment);
            fprintf(f, "---@field %s %s[]%s\n", ad->name, t, ad->optional ? "?" : "");
        }
        else if (el->kind == ELEMENT_SWITCH)
        {
            SwitchDef *sd = &el->as.sw;
            fprintf(f, "---@field %s_data table?\n", sd->field);
        }
        else if (el->kind == ELEMENT_CHUNKED)
        {
            write_cats_elements(f, &el->as.chunked,
                enums, enums_count, structs, structs_count);
        }
    }
}

void write_lua_annotations(ProtocolDef *protocols, size_t protocol_count)
{
    EnumDef *all_enums = NULL;
    size_t all_enums_count = 0;
    StructDef *all_structs = NULL;
    size_t all_structs_count = 0;
    collect_all_enums_structs(protocols, protocol_count, &all_enums, &all_enums_count,
                              &all_structs, &all_structs_count);

    ensure_dir("lua");
    FILE *f = fopen("lua/eolib.d.lua", "w");
    if (!f) { fprintf(stderr, "Failed to open lua/eolib.d.lua\n"); return; }

    fprintf(f,
        "-- eolib.d.lua\n"
        "-- LuaCATS type definitions for the eolib Lua module.\n"
        "-- Generated by tools/code_gen.c — do not edit directly.\n"
        "--\n"
        "-- Usage with lua-language-server:\n"
        "--   Add to your .luarc.json:\n"
        "--     { \"workspace.library\": [\"/path/to/eolib.d.lua\"] }\n"
        "\n"
        "---@meta\n"
        "\n"
    );

    fprintf(f,
        "---@class EoWriter\n"
        "local EoWriter = {}\n"
        "\n"
        "---@return EoWriter\n"
        "function EoWriter.new() end\n"
        "\n"
        "---@param value integer\n"
        "function EoWriter:add_byte(value) end\n"
        "---@param value integer\n"
        "function EoWriter:add_char(value) end\n"
        "---@param value integer\n"
        "function EoWriter:add_short(value) end\n"
        "---@param value integer\n"
        "function EoWriter:add_three(value) end\n"
        "---@param value integer\n"
        "function EoWriter:add_int(value) end\n"
        "---@param value string\n"
        "function EoWriter:add_string(value) end\n"
        "---@param value string\n"
        "function EoWriter:add_encoded_string(value) end\n"
        "---@param value string\n"
        "---@param length integer\n"
        "---@param padded boolean\n"
        "function EoWriter:add_fixed_string(value, length, padded) end\n"
        "---@param value string\n"
        "---@param length integer\n"
        "---@param padded boolean\n"
        "function EoWriter:add_fixed_encoded_string(value, length, padded) end\n"
        "---@param bytes string\n"
        "function EoWriter:add_bytes(bytes) end\n"
        "---@return string\n"
        "function EoWriter:to_bytes() end\n"
        "---@return integer\n"
        "function EoWriter:get_length() end\n"
        "---@param enabled boolean\n"
        "function EoWriter:set_string_sanitization_mode(enabled) end\n"
        "---@return boolean\n"
        "function EoWriter:get_string_sanitization_mode() end\n"
        "\n"
    );

    fprintf(f,
        "---@class EoReader\n"
        "local EoReader = {}\n"
        "\n"
        "---@param bytes string\n"
        "---@return EoReader\n"
        "function EoReader.new(bytes) end\n"
        "\n"
        "---@return integer\n"
        "function EoReader:get_byte() end\n"
        "---@return integer\n"
        "function EoReader:get_char() end\n"
        "---@return integer\n"
        "function EoReader:get_short() end\n"
        "---@return integer\n"
        "function EoReader:get_three() end\n"
        "---@return integer\n"
        "function EoReader:get_int() end\n"
        "---@return string\n"
        "function EoReader:get_string() end\n"
        "---@return string\n"
        "function EoReader:get_encoded_string() end\n"
        "---@param length integer\n"
        "---@return string\n"
        "function EoReader:get_fixed_string(length) end\n"
        "---@param length integer\n"
        "---@return string\n"
        "function EoReader:get_fixed_encoded_string(length) end\n"
        "---@param count integer\n"
        "---@return string\n"
        "function EoReader:get_bytes(count) end\n"
        "---@return integer\n"
        "function EoReader:remaining() end\n"
        "---@param enabled boolean\n"
        "function EoReader:set_chunked_reading_mode(enabled) end\n"
        "---@return boolean\n"
        "function EoReader:get_chunked_reading_mode() end\n"
        "function EoReader:next_chunk() end\n"
        "\n"
    );

    fprintf(f,
        "---@class EoSequencer\n"
        "local EoSequencer = {}\n"
        "\n"
        "---@param start integer\n"
        "---@return EoSequencer\n"
        "function EoSequencer.new(start) end\n"
        "\n"
        "---@return integer\n"
        "function EoSequencer:next() end\n"
        "\n"
    );

    fprintf(f,
        "---@class eolib\n"
        "---@field EoWriter EoWriter\n"
        "---@field EoReader EoReader\n"
        "---@field EoSequencer EoSequencer\n"
        "---@field CHAR_MAX integer\n"
        "---@field SHORT_MAX integer\n"
        "---@field THREE_MAX integer\n"
        "---@field INT_MAX integer\n"
        "---@field NUMBER_PADDING integer\n"
        "---@field BREAK_BYTE integer\n"
        "local eolib = {}\n"
        "\n"
        "---@param challenge integer\n"
        "---@return integer\n"
        "function eolib.server_verification_hash(challenge) end\n"
        "\n"
        "---@param bytes string\n"
        "---@param swap_multiple integer\n"
        "---@return string\n"
        "function eolib.encrypt_packet(bytes, swap_multiple) end\n"
        "\n"
        "---@param bytes string\n"
        "---@param swap_multiple integer\n"
        "---@return string\n"
        "function eolib.decrypt_packet(bytes, swap_multiple) end\n"
        "\n"
        "---@return integer\n"
        "function eolib.generate_swap_multiple() end\n"
        "\n"
        "---@param bytes string\n"
        "---@param multiple integer\n"
        "---@return string\n"
        "function eolib.swap_multiples(bytes, multiple) end\n"
        "\n"
        "---@return integer\n"
        "function eolib.generate_sequence_start() end\n"
        "\n"
        "---@param s1 integer\n"
        "---@param s2 integer\n"
        "---@return integer\n"
        "function eolib.sequence_start_from_init(s1, s2) end\n"
        "\n"
        "---@param s1 integer\n"
        "---@param s2 integer\n"
        "---@return integer\n"
        "function eolib.sequence_start_from_ping(s1, s2) end\n"
        "\n"
        "---@param start integer\n"
        "---@return integer, integer\n"
        "function eolib.sequence_init_bytes(start) end\n"
        "\n"
        "---@param start integer\n"
        "---@return integer, integer\n"
        "function eolib.sequence_ping_bytes(start) end\n"
        "\n"
        "---@param value integer\n"
        "---@return string\n"
        "function eolib.encode_number(value) end\n"
        "\n"
        "---@param bytes string\n"
        "---@return integer\n"
        "function eolib.decode_number(bytes) end\n"
        "\n"
        "---@param s string\n"
        "---@return string\n"
        "function eolib.encode_string(s) end\n"
        "\n"
        "---@param s string\n"
        "---@return string\n"
        "function eolib.decode_string(s) end\n"
        "\n"
        "---@param s string\n"
        "---@return string\n"
        "function eolib.string_to_windows_1252(s) end\n"
        "\n"
    );

    for (size_t i = 0; i < all_enums_count; ++i)
    {
        EnumDef *e = &all_enums[i];
        if (e->comment) write_lua_doc_comment(f, e->comment);
        fprintf(f, "---@enum %s\n", e->name);
        fprintf(f, "eolib.%s = {\n", e->name);
        for (size_t v = 0; v < e->values_count; ++v)
        {
            EnumValue *ev = &e->values[v];
            if (ev->comment)
            {
                char *c = xstrdup(ev->comment);
                char *s = c; while (*s && isspace((unsigned char)*s)) s++;
                char *end = s + strlen(s);
                while (end > s && isspace((unsigned char)*(end-1))) end--;
                *end = '\0';
                fprintf(f, "    %s = %d, -- %s\n", ev->name, ev->value, s);
                free(c);
            }
            else
                fprintf(f, "    %s = %d,\n", ev->name, ev->value);
        }
        fprintf(f, "}\n\n");
    }

    for (size_t p = 0; p < protocol_count; ++p)
    {
        ProtocolDef *proto = &protocols[p];
        for (size_t i = 0; i < proto->structs_count; ++i)
        {
            StructDef *s = &proto->structs[i];
            fprintf(f, "---@class %s\n", s->name);
            write_cats_elements(f, &s->elements,
                all_enums, all_enums_count,
                all_structs, all_structs_count);
            fprintf(f, "local %s = {}\n", s->name);
            fprintf(f, "\n");
            fprintf(f, "---@return %s\n", s->name);
            fprintf(f, "function %s.new() end\n", s->name);
            fprintf(f, "\n");
            fprintf(f, "---@return string\n");
            fprintf(f, "function %s:serialize() end\n", s->name);
            fprintf(f, "\n");
            fprintf(f, "---@param writer EoWriter\n");
            fprintf(f, "function %s:write(writer) end\n", s->name);
            fprintf(f, "\n");
            fprintf(f, "---@param bytes string\n");
            fprintf(f, "---@return %s\n", s->name);
            fprintf(f, "function %s.deserialize(bytes) end\n", s->name);
            fprintf(f, "\n");
            fprintf(f, "eolib.%s = %s\n\n", s->name, s->name);
        }
    }

    StringList written_packets = {0};
    for (size_t p = 0; p < protocol_count; ++p)
    {
        ProtocolDef *proto = &protocols[p];
        int is_server = strstr(proto->path, "/server/") != NULL;
        for (size_t i = 0; i < proto->packets_count; ++i)
        {
            PacketDef *pkt = &proto->packets[i];
            char name_buf[256];
            snprintf(name_buf, sizeof(name_buf), "%s%s%sPacket",
                pkt->family, pkt->action, is_server ? "Server" : "Client");

            int skip = 0;
            for (size_t w = 0; w < written_packets.count; ++w)
                if (strcmp(written_packets.items[w], name_buf) == 0) { skip = 1; break; }
            if (skip) continue;
            string_list_push(&written_packets, name_buf);

            fprintf(f, "---@class %s\n", name_buf);
            write_cats_elements(f, &pkt->elements,
                all_enums, all_enums_count,
                all_structs, all_structs_count);
            fprintf(f, "local %s = {}\n", name_buf);
            fprintf(f, "\n");
            fprintf(f, "---@return %s\n", name_buf);
            fprintf(f, "function %s.new() end\n", name_buf);
            fprintf(f, "\n");
            fprintf(f, "---@return string\n");
            fprintf(f, "function %s:serialize() end\n", name_buf);
            fprintf(f, "\n");
            fprintf(f, "---@param writer EoWriter\n");
            fprintf(f, "function %s:write(writer) end\n", name_buf);
            fprintf(f, "\n");
            fprintf(f, "---@param bytes string\n");
            fprintf(f, "---@return %s\n", name_buf);
            fprintf(f, "function %s.deserialize(bytes) end\n", name_buf);
            fprintf(f, "\n");
            fprintf(f, "eolib.%s = %s\n\n", name_buf, name_buf);
        }
    }

    fprintf(f, "return eolib\n");
    fclose(f);
    free(all_enums);
    free(all_structs);
    string_list_free(&written_packets);
}
