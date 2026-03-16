#include "code_gen_tests.h"
#include "code_gen_utils.h"
#include "code_gen_analysis.h"
#include <json-c/json.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static int base64_char_to_val(char c)
{
    if (c >= 'A' && c <= 'Z')
        return c - 'A';
    if (c >= 'a' && c <= 'z')
        return c - 'a' + 26;
    if (c >= '0' && c <= '9')
        return c - '0' + 52;
    if (c == '+')
        return 62;
    if (c == '/')
        return 63;
    return -1;
}

static int base64_decode(const char *input, uint8_t **output)
{
    size_t in_len = strlen(input);
    size_t out_len = (in_len / 4) * 3;
    if (in_len >= 1 && input[in_len - 1] == '=')
        out_len--;
    if (in_len >= 2 && input[in_len - 2] == '=')
        out_len--;

    *output = (uint8_t *)malloc(out_len + 1);
    if (!*output)
        return -1;

    size_t j = 0;
    for (size_t i = 0; i + 4 <= in_len; i += 4)
    {
        int v0 = base64_char_to_val(input[i]);
        int v1 = base64_char_to_val(input[i + 1]);
        int v2 = input[i + 2] == '=' ? 0 : base64_char_to_val(input[i + 2]);
        int v3 = input[i + 3] == '=' ? 0 : base64_char_to_val(input[i + 3]);
        if (v0 < 0 || v1 < 0)
        {
            free(*output);
            *output = NULL;
            return -1;
        }
        (*output)[j++] = (uint8_t)((v0 << 2) | (v1 >> 4));
        if (input[i + 2] != '=')
            (*output)[j++] = (uint8_t)((v1 << 4) | (v2 >> 2));
        if (input[i + 3] != '=')
            (*output)[j++] = (uint8_t)((v2 << 6) | v3);
    }

    return (int)j;
}

typedef struct
{
    char **names;
    int *has_storage;
    int *has_heap;
    size_t count;
    size_t capacity;
} PacketStorageMap;

static void packet_storage_map_push(PacketStorageMap *map, const char *name, int hs, int hh)
{
    if (map->count >= map->capacity)
    {
        map->capacity = ARRAY_GROW_CAPACITY(map->capacity);
        map->names = realloc(map->names, map->capacity * sizeof(char *));
        map->has_storage = realloc(map->has_storage, map->capacity * sizeof(int));
        map->has_heap = realloc(map->has_heap, map->capacity * sizeof(int));
        if (!map->names || !map->has_storage || !map->has_heap)
        {
            fprintf(stderr, "Out of memory\n");
            exit(1);
        }
    }
    map->names[map->count] = xstrdup(name);
    map->has_storage[map->count] = hs;
    map->has_heap[map->count] = hh;
    map->count++;
}

static int packet_storage_map_lookup(const PacketStorageMap *map, const char *name)
{
    for (size_t i = 0; i < map->count; ++i)
    {
        if (strcmp(map->names[i], name) == 0)
            return map->has_storage[i];
    }
    return 0;
}

static int packet_heap_map_lookup(const PacketStorageMap *map, const char *name)
{
    for (size_t i = 0; i < map->count; ++i)
    {
        if (strcmp(map->names[i], name) == 0)
            return map->has_heap[i];
    }
    return 0;
}

static PacketStorageMap build_packet_storage_map(ProtocolDef *protocols, size_t protocol_count)
{
    EnumDef *all_enums = NULL;
    size_t all_enums_count = 0;
    StructDef *all_structs = NULL;
    size_t all_structs_count = 0;
    collect_all_enums_structs(protocols, protocol_count, &all_enums, &all_enums_count,
                              &all_structs, &all_structs_count);

    PacketStorageMap map = {0};
    for (size_t p = 0; p < protocol_count; ++p)
    {
        ProtocolDef *protocol = &protocols[p];
        for (size_t i = 0; i < protocol->packets_count; ++i)
        {
            const char *suffix = "Client";
            if (strstr(protocol->path, "/server/") ||
                strstr(protocol->path, "/server/protocol.xml"))
            {
                suffix = "Server";
            }
            char name_buf[256];
            snprintf(name_buf, sizeof(name_buf), "%s%s%sPacket",
                     protocol->packets[i].family, protocol->packets[i].action, suffix);
            int hs = element_list_has_storage(&protocol->packets[i].elements);
            int hh = hs && element_list_has_heap(&protocol->packets[i].elements,
                                                 all_enums, all_enums_count,
                                                 all_structs, all_structs_count);
            packet_storage_map_push(&map, name_buf, hs, hh);
        }
    }

    free(all_enums);
    free(all_structs);

    return map;
}

typedef struct
{
    char *struct_name;
    char *field_name;
    int size;
} FixedArrayEntry;

typedef struct
{
    FixedArrayEntry *entries;
    size_t count;
    size_t capacity;
} FixedArrayMap;

static void fixed_array_map_push(FixedArrayMap *map, const char *struct_name,
                                 const char *field_name, int size)
{
    if (map->count >= map->capacity)
    {
        map->capacity = ARRAY_GROW_CAPACITY(map->capacity);
        map->entries = realloc(map->entries, map->capacity * sizeof(FixedArrayEntry));
        if (!map->entries)
        {
            fprintf(stderr, "Out of memory\n");
            exit(1);
        }
    }
    map->entries[map->count].struct_name = xstrdup(struct_name ? struct_name : "");
    map->entries[map->count].field_name = xstrdup(field_name ? field_name : "");
    map->entries[map->count].size = size;
    map->count++;
}

static int fixed_array_lookup(const FixedArrayMap *map, const char *struct_name,
                              const char *field_name)
{
    if (!struct_name || !field_name)
        return 0;
    for (size_t i = 0; i < map->count; ++i)
    {
        if (strcmp(map->entries[i].struct_name, struct_name) == 0 &&
            strcmp(map->entries[i].field_name, field_name) == 0)
        {
            return map->entries[i].size;
        }
    }
    return 0;
}

static void collect_fixed_arrays_from_elements(const char *struct_name,
                                               ElementList *elements,
                                               FixedArrayMap *map);

static void collect_fixed_arrays_from_elements(const char *struct_name,
                                               ElementList *elements,
                                               FixedArrayMap *map)
{
    for (size_t i = 0; i < elements->elements_count; ++i)
    {
        StructElement *el = &elements->elements[i];
        if (el->kind == ELEMENT_ARRAY)
        {
            ArrayDef *arr = &el->as.array;
            if (arr->name && is_static_length(arr->length))
            {
                char *field_name = to_snake_case(arr->name);
                int size = atoi(arr->length);
                fixed_array_map_push(map, struct_name, field_name, size);
                free(field_name);
            }
        }
        else if (el->kind == ELEMENT_CHUNKED)
        {
            collect_fixed_arrays_from_elements(struct_name, &el->as.chunked, map);
        }
        else if (el->kind == ELEMENT_SWITCH)
        {
            SwitchDef *sw = &el->as.sw;
            for (size_t c = 0; c < sw->cases_count; ++c)
            {
                if (sw->cases[c].has_elements)
                {
                    collect_fixed_arrays_from_elements(struct_name,
                                                       &sw->cases[c].elements, map);
                }
            }
        }
    }
}

static void build_fixed_array_map(ProtocolDef *protocols, size_t protocol_count,
                                  FixedArrayMap *map)
{
    for (size_t p = 0; p < protocol_count; ++p)
    {
        ProtocolDef *proto = &protocols[p];
        for (size_t s = 0; s < proto->structs_count; ++s)
        {
            collect_fixed_arrays_from_elements(proto->structs[s].name,
                                               &proto->structs[s].elements, map);
        }
        for (size_t pk = 0; pk < proto->packets_count; ++pk)
        {
            PacketDef *pkt = &proto->packets[pk];
            const char *dir_suffix = (strstr(proto->path, "/server/") ||
                                      strstr(proto->path, "/server/protocol.xml"))
                                         ? "Server"
                                         : "Client";
            char pkt_struct[256];
            snprintf(pkt_struct, sizeof(pkt_struct), "%s%s%sPacket",
                     pkt->family, pkt->action, dir_suffix);
            collect_fixed_arrays_from_elements(pkt_struct, &pkt->elements, map);
        }
    }
}

static void collect_known_structs(ProtocolDef *protocols, size_t protocol_count,
                                  StringList *out)
{
    for (size_t p = 0; p < protocol_count; ++p)
    {
        for (size_t s = 0; s < protocols[p].structs_count; ++s)
        {
            string_list_push_unique(out, protocols[p].structs[s].name);
        }
    }
}

static const char *strip_type_namespace(const char *type_str)
{
    const char *last = NULL;
    const char *p = type_str;
    while (*p)
    {
        if (p[0] == ':' && p[1] == ':')
        {
            last = p + 2;
            p += 2;
        }
        else
        {
            ++p;
        }
    }
    return last ? last : type_str;
}

static char *extract_union_variant(const char *local_type, const char *field_name)
{
    char *pascal_field = to_pascal_case(field_name);
    const char *pos = strstr(local_type, pascal_field);
    char *variant;
    if (pos)
    {
        const char *suffix = pos + strlen(pascal_field);
        variant = to_snake_case(suffix);
    }
    else
    {
        variant = to_snake_case(local_type);
    }
    free(pascal_field);
    return variant;
}

static void write_property_assertions(FILE *f, struct json_object *properties,
                                      const char *c_path, const char *assert_path,
                                      const char *current_struct,
                                      StringList *known_structs,
                                      FixedArrayMap *fixed_arrays);

static void write_single_property_assertions(FILE *f, struct json_object *prop,
                                             const char *c_path,
                                             const char *assert_path,
                                             const char *current_struct,
                                             StringList *known_structs,
                                             FixedArrayMap *fixed_arrays)
{
    struct json_object *name_obj = NULL, *type_obj = NULL;
    struct json_object *value_obj = NULL, *children_obj = NULL;
    json_object_object_get_ex(prop, "name", &name_obj);
    json_object_object_get_ex(prop, "type", &type_obj);
    json_object_object_get_ex(prop, "value", &value_obj);
    json_object_object_get_ex(prop, "children", &children_obj);

    const char *name = name_obj ? json_object_get_string(name_obj) : NULL;
    const char *type_str = type_obj ? json_object_get_string(type_obj) : NULL;

    if (!name)
        return;

    struct json_object *optional_obj = NULL;
    json_object_object_get_ex(prop, "optional", &optional_obj);
    int is_optional = optional_obj && json_object_get_boolean(optional_obj);

    char c_expr[2048];
    if (is_optional && type_str &&
        strcmp(type_str, "string") != 0 && type_str[0] != '[')
        snprintf(c_expr, sizeof(c_expr), "*%s%s", c_path, name);
    else
        snprintf(c_expr, sizeof(c_expr), "%s%s", c_path, name);

    char label[1024];
    snprintf(label, sizeof(label), "%s %s", assert_path, name);

    if (value_obj && type_str)
    {
        if (strcmp(type_str, "string") == 0)
        {
            const char *str_val = json_object_get_string(value_obj);
            fprintf(f, "    expect_equal_str(\"%s\", %s, ", label, c_expr);
            write_escaped_c_string(f, str_val ? str_val : "");
            fprintf(f, ");\n");
        }
        else if (strcmp(type_str, "bool") == 0)
        {
            int bool_val = json_object_get_boolean(value_obj);
            fprintf(f, "    expect_equal_int(\"%s\", (int)%s, %d);\n",
                    label, c_expr, bool_val);
        }
        else if (strcmp(type_str, "[]byte") == 0)
        {
            const char *b64 = json_object_get_string(value_obj);
            if (b64 && *b64)
            {
                uint8_t *blob = NULL;
                int blob_len = base64_decode(b64, &blob);
                if (blob_len >= 0)
                {
                    fprintf(f, "    {\n");
                    fprintf(f, "        static const uint8_t expected_bytes[] = ");
                    write_byte_array_literal(f, blob, (size_t)blob_len);
                    fprintf(f, ";\n");
                    fprintf(f,
                            "        expect_equal_bytes(\"%s\", %s, expected_bytes, %d);\n",
                            label, c_expr, blob_len);
                    fprintf(f,
                            "        expect_equal_size(\"%s length\", %s%s_length, %zu);\n",
                            label, c_path, name, (size_t)blob_len);
                    fprintf(f, "    }\n");
                    free(blob);
                }
            }
        }
        else
        {
            int64_t num_val = json_object_get_int64(value_obj);
            fprintf(f, "    expect_equal_int(\"%s\", (int)%s, %d);\n",
                    label, c_expr, (int)num_val);
        }
    }

    if (type_str && type_str[0] == '[' && type_str[1] == ']' && strcmp(type_str + 2, "byte") != 0)
    {
        int child_count = children_obj ? json_object_array_length(children_obj) : 0;
        const char *elem_type = type_str + 2;

        if (strcmp(elem_type, "int") == 0)
        {
            int fixed_size = fixed_array_lookup(fixed_arrays, current_struct, name);
            if (fixed_size > 0)
            {
                for (int j = 0; j < child_count; ++j)
                {
                    struct json_object *child = json_object_array_get_idx(children_obj, j);
                    struct json_object *child_val = NULL;
                    json_object_object_get_ex(child, "value", &child_val);
                    if (child_val)
                    {
                        int64_t v = json_object_get_int64(child_val);
                        fprintf(f, "    expect_equal_int(\"%s[%d]\", (int)%s[%d], %d);\n",
                                label, j, c_expr, j, (int)v);
                    }
                }
            }
            else
            {
                fprintf(f, "    expect_equal_size(\"%s length\", %s%s_length, %d);\n",
                        label, c_path, name, child_count);
                for (int j = 0; j < child_count; ++j)
                {
                    struct json_object *child = json_object_array_get_idx(children_obj, j);
                    struct json_object *child_val = NULL;
                    json_object_object_get_ex(child, "value", &child_val);
                    if (child_val)
                    {
                        int64_t v = json_object_get_int64(child_val);
                        fprintf(f, "    expect_equal_int(\"%s[%d]\", (int)%s[%d], %d);\n",
                                label, j, c_expr, j, (int)v);
                    }
                }
            }
        }
        else if (strcmp(elem_type, "string") == 0)
        {
            int fixed_size = fixed_array_lookup(fixed_arrays, current_struct, name);
            if (fixed_size > 0)
            {
                for (int j = 0; j < child_count; ++j)
                {
                    struct json_object *child = json_object_array_get_idx(children_obj, j);
                    struct json_object *child_val = NULL;
                    json_object_object_get_ex(child, "value", &child_val);
                    if (child_val)
                    {
                        const char *sv = json_object_get_string(child_val);
                        fprintf(f, "    expect_equal_str(\"%s[%d]\", %s[%d], ",
                                label, j, c_expr, j);
                        write_escaped_c_string(f, sv ? sv : "");
                        fprintf(f, ");\n");
                    }
                }
            }
            else
            {
                fprintf(f, "    expect_equal_size(\"%s length\", %s%s_length, %d);\n",
                        label, c_path, name, child_count);
                for (int j = 0; j < child_count; ++j)
                {
                    struct json_object *child = json_object_array_get_idx(children_obj, j);
                    struct json_object *child_val = NULL;
                    json_object_object_get_ex(child, "value", &child_val);
                    if (child_val)
                    {
                        const char *sv = json_object_get_string(child_val);
                        fprintf(f, "    expect_equal_str(\"%s[%d]\", %s[%d], ",
                                label, j, c_expr, j);
                        write_escaped_c_string(f, sv ? sv : "");
                        fprintf(f, ");\n");
                    }
                }
            }
        }
        else
        {
            const char *local_elem = strip_type_namespace(elem_type);
            int fixed_size = fixed_array_lookup(fixed_arrays, current_struct, name);

            if (fixed_size > 0)
            {
                for (int j = 0; j < child_count; ++j)
                {
                    struct json_object *child = json_object_array_get_idx(children_obj, j);
                    struct json_object *child_children = NULL;
                    json_object_object_get_ex(child, "children", &child_children);

                    char elem_c_path[2048];
                    snprintf(elem_c_path, sizeof(elem_c_path), "%s%s[%d].", c_path, name, j);

                    char elem_label[1024];
                    snprintf(elem_label, sizeof(elem_label), "%s[%d]", label, j);

                    if (child_children)
                    {
                        write_property_assertions(f, child_children, elem_c_path,
                                                  elem_label, local_elem,
                                                  known_structs, fixed_arrays);
                    }
                }
            }
            else
            {
                fprintf(f, "    expect_equal_size(\"%s length\", %s%s_length, %d);\n",
                        label, c_path, name, child_count);

                // Bail at run-time if length == 0 to avoid generating invalid C expressions like packet.array[0]
                fprintf(f, "    if (%s%s_length > 0)\n    {\n", c_path, name);

                for (int j = 0; j < child_count; ++j)
                {
                    struct json_object *child = json_object_array_get_idx(children_obj, j);
                    struct json_object *child_children = NULL;
                    json_object_object_get_ex(child, "children", &child_children);

                    char elem_c_path[2048];
                    snprintf(elem_c_path, sizeof(elem_c_path), "%s[%d].", c_expr, j);

                    char elem_label[1024];
                    snprintf(elem_label, sizeof(elem_label), "%s[%d]", label, j);

                    if (child_children)
                    {
                        write_property_assertions(f, child_children, elem_c_path,
                                                  elem_label, local_elem,
                                                  known_structs, fixed_arrays);
                    }
                }

                fprintf(f, "    }\n");
            }
        }
    }
    else if (children_obj && type_str)
    {
        int child_count = json_object_array_length(children_obj);
        (void)child_count;

        const char *local_type = strip_type_namespace(type_str);
        char child_c_path[2048];
        const char *child_struct;

        if (string_list_contains(known_structs, local_type))
        {
            snprintf(child_c_path, sizeof(child_c_path), "%s%s.", c_path, name);
            child_struct = local_type;
        }
        else
        {
            char *variant = extract_union_variant(local_type, name);
            snprintf(child_c_path, sizeof(child_c_path),
                     "%s%s->%s.", c_path, name, variant);
            free(variant);
            child_struct = current_struct;
        }

        write_property_assertions(f, children_obj, child_c_path, label,
                                  child_struct, known_structs, fixed_arrays);
    }
}

static void write_property_assertions(FILE *f, struct json_object *properties,
                                      const char *c_path, const char *assert_path,
                                      const char *current_struct,
                                      StringList *known_structs,
                                      FixedArrayMap *fixed_arrays)
{
    if (!properties)
        return;
    int n = json_object_array_length(properties);
    for (int i = 0; i < n; ++i)
    {
        struct json_object *prop = json_object_array_get_idx(properties, i);
        write_single_property_assertions(f, prop, c_path, assert_path,
                                         current_struct, known_structs, fixed_arrays);
    }
}

void write_packet_tests(ProtocolDef *protocols, size_t protocol_count)
{
    if (ensure_dir("tests") != 0)
    {
        fprintf(stderr, "Failed to create tests directory\n");
        return;
    }

    FILE *f = fopen("tests/packet_tests.c", "w");
    if (!f)
    {
        fprintf(stderr, "Failed to open tests/packet_tests.c for writing\n");
        return;
    }

    FILE *list_file = fopen("tests/packet_tests.list", "w");
    if (!list_file)
    {
        fprintf(stderr, "Failed to open tests/packet_tests.list for writing\n");
        fclose(f);
        return;
    }

    PacketStorageMap map = build_packet_storage_map(protocols, protocol_count);

    StringList known_structs = {0};
    collect_known_structs(protocols, protocol_count, &known_structs);

    FixedArrayMap fixed_arrays = {0};
    build_fixed_array_map(protocols, protocol_count, &fixed_arrays);

    fprintf(f, "%s", CODEGEN_WARNING);
    fprintf(f, "#include \"test_utils.h\"\n");
    fprintf(f, "#include \"eolib/data.h\"\n");
    fprintf(f, "#include \"eolib/protocol.h\"\n");
    fprintf(f, "#include <stdio.h>\n");
    fprintf(f, "#include <stdlib.h>\n");
    fprintf(f, "#include <string.h>\n\n");

    StringList test_functions = {0};
    StringList test_case_names = {0};
    const char *dirs[] = {"client", "server"};
    for (int d = 0; d < 2; ++d)
    {
        char dir_path[256];
        snprintf(dir_path, sizeof(dir_path), "eo-captured-packets/%s", dirs[d]);

        StringList json_paths = {0};
        list_json_files_in_dir(dir_path, &json_paths);

        for (size_t i = 0; i < json_paths.count; ++i)
        {
            const char *path = json_paths.items[i];

            struct json_object *root = json_object_from_file(path);
            if (!root)
            {
                fprintf(stderr, "Failed to parse JSON: %s\n", path);
                continue;
            }

            struct json_object *family_obj = NULL, *action_obj = NULL, *expected_obj = NULL;
            struct json_object *properties_obj = NULL;
            if (!json_object_object_get_ex(root, "family", &family_obj) ||
                !json_object_object_get_ex(root, "action", &action_obj) ||
                !json_object_object_get_ex(root, "expected", &expected_obj))
            {
                fprintf(stderr, "Missing fields in JSON: %s\n", path);
                json_object_put(root);
                continue;
            }

            const char *family = json_object_get_string(family_obj);
            const char *action = json_object_get_string(action_obj);
            const char *expected_b64 = json_object_get_string(expected_obj);
            json_object_object_get_ex(root, "properties", &properties_obj);

            const char *suffix = d == 0 ? "Client" : "Server";
            char packet_name[256];
            snprintf(packet_name, sizeof(packet_name), "%s%s%sPacket",
                     family, action, suffix);

            int has_storage = packet_storage_map_lookup(&map, packet_name);

            uint8_t *expected_bytes = NULL;
            int expected_len = base64_decode(expected_b64, &expected_bytes);

            const char *basename = strrchr(path, '/');
            basename = basename ? basename + 1 : path;
            size_t stem_len = strlen(basename);
            if (stem_len > 5 && strcmp(basename + stem_len - 5, ".json") == 0)
                stem_len -= 5;
            char *stem = (char *)malloc(stem_len + 1);
            memcpy(stem, basename, stem_len);
            stem[stem_len] = '\0';

            char fn_name[512];
            snprintf(fn_name, sizeof(fn_name), "test_packet_%s_%s", dirs[d], stem);

            const char *name_start = fn_name;
            if (strncmp(name_start, "test_", 5) == 0)
                name_start += 5;
            string_list_push(&test_case_names, name_start);
            fprintf(list_file, "%s\n", name_start);

            fprintf(f, "static void %s(void)\n{\n", fn_name);

            if (expected_len > 0)
            {
                fprintf(f, "    static const uint8_t expected[] = ");
                write_byte_array_literal(f, expected_bytes, (size_t)expected_len);
                fprintf(f, ";\n");
                fprintf(f, "    size_t expected_len = sizeof(expected);\n");
                fprintf(f, "    EoReader reader = eo_reader_init(expected, expected_len);\n");
            }
            else
            {
                fprintf(f, "    size_t expected_len = 0;\n");
                fprintf(f, "    EoReader reader = eo_reader_init(NULL, 0);\n");
            }
            fprintf(f, "\n");

            int has_heap = packet_heap_map_lookup(&map, packet_name);
            (void)has_heap; /* eo_free is safe to call even when there is nothing to free */

            fprintf(f, "    %s packet = %s_init();\n", packet_name, packet_name);
            fprintf(f,
                    "    int deser_result = eo_deserialize((EoSerialize *)&packet, &reader);\n");
            fprintf(f,
                    "    expect(\"%s_deserialize\", deser_result != -1);\n",
                    packet_name);
            fprintf(f, "    if (deser_result == -1) return;\n");
            fprintf(f, "\n");
            if (has_storage && properties_obj)
            {
                write_property_assertions(f, properties_obj, "packet.",
                                          name_start, packet_name,
                                          &known_structs, &fixed_arrays);
                fprintf(f, "\n");
            }
            fprintf(f, "    EoWriter writer = eo_writer_init_with_capacity("
                       "eo_get_size((const EoSerialize *)&packet));\n");
            fprintf(f,
                    "    expect(\"%s_serialize\","
                    " eo_serialize((const EoSerialize *)&packet, &writer) != -1);\n",
                    packet_name);
            if (expected_len > 0)
            {
                fprintf(f,
                        "    expect_equal_bytes(\"%s roundtrip\","
                        " writer.data, expected, expected_len);\n",
                        packet_name);
                fprintf(f,
                        "    expect_equal_size(\"%s roundtrip length\","
                        " writer.length, expected_len);\n",
                        packet_name);
            }
            fprintf(f, "    eo_free((EoSerialize *)&packet);\n");

            fprintf(f, "    free(writer.data);\n");
            fprintf(f, "}\n\n");

            string_list_push(&test_functions, fn_name);

            free(stem);
            if (expected_bytes)
                free(expected_bytes);
            json_object_put(root);
        }
    }

    fprintf(f, "static const TestCase packet_tests[] = {\n");
    for (size_t i = 0; i < test_functions.count; ++i)
    {
        fprintf(f, "    {\"%s\", %s},\n",
                test_case_names.items[i], test_functions.items[i]);
    }
    fprintf(f, "};\n\n");
    fprintf(f, "int main(int argc, char **argv)\n{\n");
    fprintf(f, "    return run_tests(packet_tests, sizeof(packet_tests) / sizeof(packet_tests[0]), argc, argv);\n");
    fprintf(f, "}\n");

    fclose(f);
    fclose(list_file);
}
